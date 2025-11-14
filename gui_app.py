#!/usr/bin/env python3


import sys
import os
import json
import subprocess
import tempfile
from pathlib import Path
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QFileDialog, QSlider, QSpinBox, QGroupBox,
    QRadioButton, QMessageBox, QProgressBar, QStatusBar
)
from PyQt6.QtCore import Qt, QPoint, QThread, pyqtSignal
from PyQt6.QtGui import QImage, QPixmap, QPainter, QPen, QColor
import numpy as np
from PIL import Image
import cv2


class SegmentationWorker(QThread):
    """Background thread for running C++ segmentation"""
    finished = pyqtSignal(bool, str)
    progress = pyqtSignal(str)
    
    def __init__(self, exe_path, image_path, width, height, seed_path, output_path):
        super().__init__()
        self.exe_path = exe_path
        self.image_path = image_path
        self.width = width
        self.height = height
        self.seed_path = seed_path
        self.output_path = output_path
    
    def run(self):
        try:
            self.progress.emit("Running segmentation...")
            #  segment.exe image.bin W H mask seed.bin out_mask.bin
            cmd = [
                self.exe_path,
                self.image_path,
                str(self.width),
                str(self.height),
                "mask",
                self.seed_path,
                self.output_path
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60
            )
            
            if result.returncode == 0:
                self.finished.emit(True, "Segmentation completed successfully!")
            else:
                self.finished.emit(False, f"Error: {result.stderr}")
        except subprocess.TimeoutExpired:
            self.finished.emit(False, "Segmentation timed out")
        except Exception as e:
            self.finished.emit(False, f"Error: {str(e)}")


class ImageCanvas(QLabel):
    #draw the screen
    
    def __init__(self):
        super().__init__()
        self.setMinimumSize(800, 600)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet("QLabel { background-color: #2b2b2b; }")
        
        self.image = None
        self.overlay = None
        self.drawing = False
        self.last_point = None
        self.fg_scribbles = []
        self.bg_scribbles = []
        self.brush_mode = "fg"  # "fg" or "bg"
        self.brush_size = 3
        
    def load_image(self, path):
        """Load and display image"""
        self.image = QImage(path)
        if self.image.isNull():
            return False
        
        # Create overlay for scribbles
        self.overlay = QImage(self.image.size(), QImage.Format.Format_ARGB32)
        self.overlay.fill(Qt.GlobalColor.transparent)
        
        self.fg_scribbles = []
        self.bg_scribbles = []
        self.update_display()
        return True
    
    def update_display(self):
        """Refresh the display with image + overlay"""
        if self.image is None:
            return
        
        # Combine image and overlay
        combined = QImage(self.image)
        painter = QPainter(combined)
        painter.drawImage(0, 0, self.overlay)
        painter.end()
        
        # Scale to fit widget
        scaled = combined.scaled(
            self.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self.setPixmap(QPixmap.fromImage(scaled))
    
    def get_image_coords(self, widget_pos):
        """Convert widget coordinates to image coordinates"""
        if self.image is None:
            return None
        
        pixmap = self.pixmap()
        if pixmap is None:
            return None
        
        # Calculate offset for centered image
        offset_x = (self.width() - pixmap.width()) // 2
        offset_y = (self.height() - pixmap.height()) // 2
        
        # Map to image coordinates
        x = int((widget_pos.x() - offset_x) * self.image.width() / pixmap.width())
        y = int((widget_pos.y() - offset_y) * self.image.height() / pixmap.height())
        
        if 0 <= x < self.image.width() and 0 <= y < self.image.height():
            return QPoint(x, y)
        return None
    
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.drawing = True
            self.last_point = self.get_image_coords(event.pos())
            if self.last_point:
                self.draw_point(self.last_point)
    
    def mouseMoveEvent(self, event):
        if self.drawing and self.last_point is not None:
            current_point = self.get_image_coords(event.pos())
            if current_point:
                self.draw_line(self.last_point, current_point)
                self.last_point = current_point
    
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.drawing = False
            self.last_point = None
    
    def draw_point(self, point):
        """Draw a single point"""
        painter = QPainter(self.overlay)
        pen = QPen(QColor(0, 255, 0, 200) if self.brush_mode == "fg" else QColor(255, 0, 0, 200))
        pen.setWidth(self.brush_size)
        pen.setCapStyle(Qt.PenCapStyle.RoundCap)
        painter.setPen(pen)
        painter.drawPoint(point)
        painter.end()
        
        # Store scribble
        scribbles = self.fg_scribbles if self.brush_mode == "fg" else self.bg_scribbles
        scribbles.append([point.x(), point.y()])
        
        self.update_display()
    
    def draw_line(self, start, end):
        """Draw a line from start to end"""
        painter = QPainter(self.overlay)
        pen = QPen(QColor(0, 255, 0, 200) if self.brush_mode == "fg" else QColor(255, 0, 0, 200))
        pen.setWidth(self.brush_size)
        pen.setCapStyle(Qt.PenCapStyle.RoundCap)
        painter.setPen(pen)
        painter.drawLine(start, end)
        painter.end()
        
        # Store scribble
        scribbles = self.fg_scribbles if self.brush_mode == "fg" else self.bg_scribbles
        scribbles.append([end.x(), end.y()])
        
        self.update_display()
    
    def clear_scribbles(self):
        """Clear all scribbles"""
        self.fg_scribbles = []
        self.bg_scribbles = []
        if self.overlay:
            self.overlay.fill(Qt.GlobalColor.transparent)
            self.update_display()


class MainWindow(QMainWindow):
    """Main screem"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("reImage - Interactive Segmentation")
        self.setGeometry(100, 100, 1200, 800)
        
        self.image_path = None
        self.temp_dir = tempfile.mkdtemp()
        
        #runn segment
        if getattr(sys, 'frozen', False):
        
            base_path = Path(sys._MEIPASS)
            self.cpp_exe = str(base_path / "segment.exe")
        else:
            # Running as script
            self.cpp_exe = str(Path(__file__).parent / "cpp" / "build" / "segment.exe")
        
        self.init_ui()
    
    def init_ui(self):
        """Initialize the user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        main_layout = QHBoxLayout(central_widget)
        
        # Left panel - Canvas
        self.canvas = ImageCanvas()
        main_layout.addWidget(self.canvas, stretch=3)
        
        # Right panel - Controls
        right_panel = QVBoxLayout()
        
        # File controls
        file_group = QGroupBox("File")
        file_layout = QVBoxLayout()
        
        self.open_btn = QPushButton("📂 Open Image")
        self.open_btn.clicked.connect(self.open_image)
        file_layout.addWidget(self.open_btn)
        
        self.save_btn = QPushButton("💾")
        self.save_btn.clicked.connect(self.save_result)
        self.save_btn.setEnabled(False)
        file_layout.addWidget(self.save_btn)
        
        file_group.setLayout(file_layout)
        right_panel.addWidget(file_group)
        
        # Drawing controls
        draw_group = QGroupBox("Drawing Tools")
        draw_layout = QVBoxLayout()
        
        self.fg_radio = QRadioButton("🟢 Foreground")
        self.fg_radio.setChecked(True)
        self.fg_radio.toggled.connect(lambda: self.set_brush_mode("fg"))
        draw_layout.addWidget(self.fg_radio)
        
        self.bg_radio = QRadioButton("🔴 Background")
        self.bg_radio.toggled.connect(lambda: self.set_brush_mode("bg"))
        draw_layout.addWidget(self.bg_radio)
        
        # Brush size
        brush_layout = QHBoxLayout()
        brush_layout.addWidget(QLabel("Brush Size:"))
        self.brush_slider = QSlider(Qt.Orientation.Horizontal)
        self.brush_slider.setMinimum(1)
        self.brush_slider.setMaximum(20)
        self.brush_slider.setValue(3)
        self.brush_slider.valueChanged.connect(self.update_brush_size)
        brush_layout.addWidget(self.brush_slider)
        self.brush_label = QLabel("3")
        brush_layout.addWidget(self.brush_label)
        draw_layout.addLayout(brush_layout)
        
        self.clear_btn = QPushButton("🗑️ Clear Scribbles")
        self.clear_btn.clicked.connect(self.canvas.clear_scribbles)
        draw_layout.addWidget(self.clear_btn)
        
        draw_group.setLayout(draw_layout)
        right_panel.addWidget(draw_group)
        
        # Segmentation controls
        seg_group = QGroupBox("Segmentation")
        seg_layout = QVBoxLayout()
        
        self.segment_btn = QPushButton("⚡ Run Segmentation")
        self.segment_btn.clicked.connect(self.run_segmentation)
        self.segment_btn.setEnabled(False)
        self.segment_btn.setStyleSheet("QPushButton { font-weight: bold; padding: 10px; }")
        seg_layout.addWidget(self.segment_btn)
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        seg_layout.addWidget(self.progress_bar)
        
        seg_group.setLayout(seg_layout)
        right_panel.addWidget(seg_group)
        
        right_panel.addStretch()
        
        
        info_label = QLabel(
            "Team Members\n"
            "1. Rupesh \n"
            "2. Tushar Verma\n"
            "3. Rewant Rai\n"
            "4. Paaras Kataria"
        )
        info_label.setStyleSheet("QLabel { color: #888; padding: 10px; }")
        info_label.setWordWrap(True)
        right_panel.addWidget(info_label)
        
        main_layout.addLayout(right_panel, stretch=1)
        
        # Status bar
        self.statusBar().showMessage("Ready - Open an image to begin")
    
    def open_image(self):
        """Open an image file"""
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open Image",
            "",
            "Image Files (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
        )
        
        if file_path:
            if self.canvas.load_image(file_path):
                self.image_path = file_path
                self.segment_btn.setEnabled(True)
                self.statusBar().showMessage(f"Loaded: {Path(file_path).name}")
            else:
                QMessageBox.critical(self, "Error", "Failed to load image")
    
    def set_brush_mode(self, mode):
        """Set foreground or background brush mode"""
        self.canvas.brush_mode = mode
    
    def update_brush_size(self, value):
        """Update brush size"""
        self.canvas.brush_size = value
        self.brush_label.setText(str(value))
    
    def run_segmentation(self):
        """Run the C++ segmentation backend"""
        if not self.image_path:
            return
        
        if not self.canvas.fg_scribbles and not self.canvas.bg_scribbles:
            QMessageBox.warning(
                self,
                "No Scribbles",
                "Please draw some foreground (green) and background (red) scribbles first!"
            )
            return
        
        # Prepare temporary files
        self.statusBar().showMessage("Preparing data...")
        
        try:
            # Get image dimensions
            img = Image.open(self.image_path)
            W, H = img.size
            
            # Export image to binary
            image_bin = os.path.join(self.temp_dir, "image.bin")
            self.export_image_binary(self.image_path, image_bin, W, H)
            
            # Export seed mask with scribbles
            seed_bin = os.path.join(self.temp_dir, "seed.bin")
            self.export_seed_mask(seed_bin, W, H)
            
            
            output_bin = os.path.join(self.temp_dir, "output_mask.bin")
            
            # Run segmentation in background
            self.segment_btn.setEnabled(False)
            self.progress_bar.setVisible(True)
            self.progress_bar.setRange(0, 0)  # Indeterminate
            
            self.worker = SegmentationWorker(
                self.cpp_exe, image_bin, W, H, seed_bin, output_bin
            )
            self.worker.finished.connect(self.on_segmentation_finished)
            self.worker.progress.connect(self.statusBar().showMessage)
            self.worker.start()
            
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to run segmentation: {str(e)}")
            self.segment_btn.setEnabled(True)
            self.progress_bar.setVisible(False)
    
    def export_image_binary(self, image_path, output_path, W, H):
        """Export image as binary file for C++"""
        img = Image.open(image_path).convert('RGB')
        data = np.array(img, dtype=np.uint8)
        data.tofile(output_path)
    
    def export_seed_mask(self, output_path, W, H):
        """Export seed mask with scribbles painted"""
        # Initialize mask: 128 = unknown, 255 = foreground seed, 0 = background seed
        mask = np.full((H, W), 128, dtype=np.uint8)
        
        # Paint foreground scribbles
        for x, y in self.canvas.fg_scribbles:
            if 0 <= x < W and 0 <= y < H:
                # Paint a small circle around each scribble point
                for dy in range(-self.canvas.brush_size, self.canvas.brush_size + 1):
                    for dx in range(-self.canvas.brush_size, self.canvas.brush_size + 1):
                        if dx*dx + dy*dy <= self.canvas.brush_size * self.canvas.brush_size:
                            px, py = x + dx, y + dy
                            if 0 <= px < W and 0 <= py < H:
                                mask[py, px] = 255
        
        # Paint background scribbles
        for x, y in self.canvas.bg_scribbles:
            if 0 <= x < W and 0 <= y < H:
                for dy in range(-self.canvas.brush_size, self.canvas.brush_size + 1):
                    for dx in range(-self.canvas.brush_size, self.canvas.brush_size + 1):
                        if dx*dx + dy*dy <= self.canvas.brush_size * self.canvas.brush_size:
                            px, py = x + dx, y + dy
                            if 0 <= px < W and 0 <= py < H:
                                mask[py, px] = 0
        
        mask.tofile(output_path)
    
    def on_segmentation_finished(self, success, message):
        """Handle segmentation completion"""
        self.progress_bar.setVisible(False)
        self.segment_btn.setEnabled(True)
        
        if success:
            self.statusBar().showMessage(message)
            self.save_btn.setEnabled(True)
            
            
            try:
                self.display_result()
                QMessageBox.information(self, "Success", "Segmentation completed!\n")
            except Exception as e:
                QMessageBox.warning(self, "Success", f"Segmentation completed but failed to display result:\n{str(e)}")
        else:
            self.statusBar().showMessage("Segmentation failed")
            QMessageBox.critical(self, "Error", message)
    
    def display_result(self):
        """Display the segmentation result as an overlay on the canvas"""
        output_mask = os.path.join(self.temp_dir, "output_mask.bin")
        img = Image.open(self.image_path)
        W, H = img.size
        
        mask = np.fromfile(output_mask, dtype=np.uint8).reshape((H, W))
        
        # Create overlay with green tint on segmented region
        img_arr = np.array(img)
        overlay = img_arr.copy()
        overlay[mask == 1] = (overlay[mask == 1] * 0.6 + np.array([0, 255, 0]) * 0.4).astype(np.uint8)
        
        # Convert to QImage - need to copy data to avoid memory issues
        height, width, channel = overlay.shape
        bytes_per_line = 3 * width
        # Make a copy of the data to ensure it persists
        overlay_copy = np.ascontiguousarray(overlay)
        q_img = QImage(overlay_copy.data, width, height, bytes_per_line, QImage.Format.Format_RGB888).copy()
        
        # Update canvas
        self.canvas.image = q_img
        self.canvas.overlay.fill(Qt.GlobalColor.transparent)  
        self.canvas.update_display()
    
    def save_result(self):
        """Save the segmentation result"""
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Save Result",
            "output.png",
            "PNG Image (*.png);;JPEG Image (*.jpg);;All Files (*)"
        )
        
        if file_path:
            try:
                output_mask = os.path.join(self.temp_dir, "output_mask.bin")
                img = Image.open(self.image_path)
                W, H = img.size
                
                mask = np.fromfile(output_mask, dtype=np.uint8).reshape((H, W))
                
                # Create overlay
                img_arr = np.array(img)
                overlay = img_arr.copy()
                overlay[mask == 1] = overlay[mask == 1] * 0.5 + np.array([0, 255, 0]) * 0.5
                
                result = Image.fromarray(overlay.astype(np.uint8))
                result.save(file_path)
                
                self.statusBar().showMessage(f"Saved: {file_path}")
                QMessageBox.information(self, "Saved", f"Result saved to:\n{file_path}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save: {str(e)}")


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')  
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
