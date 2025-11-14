#!/usr/bin/env python3

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import cv2
import numpy as np
from PIL import Image
from PyQt6.QtCore import QPoint, Qt, QThread, pyqtSignal
from PyQt6.QtGui import QColor, QImage, QPainter, QPen, QPixmap
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QProgressBar,
    QPushButton,
    QRadioButton,
    QSlider,
    QSpinBox,
    QStatusBar,
    QVBoxLayout,
    QWidget,
)


class SegmentationWorker(QThread):
    """
    BG worker thread - so the UI doesn't freeze like my laptop during a Teams call
    Runs the C++ segmentation binary and returns back
    """

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
            # run segment.exe image.bin W H mask seed.bin out_mask.bin
            cmd = [
                self.exe_path,
                self.image_path,
                str(self.width),
                str(self.height),
                "mask",
                self.seed_path,
                self.output_path,
            ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60,  # Give it a minute max, ain't got all day
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
    # scribbling
    #

    def __init__(self):
        super().__init__()
        self.setMinimumSize(800, 600)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet("QLabel { background-color: #2b2b2b; }")  # Dark mode ftw

        self.image = None
        self.overlay = None  # Transparent layer for our artistic masterpieces
        self.drawing = False
        self.last_point = None
        self.fg_scribbles = []  # Green stuff = "keep this part"
        self.bg_scribbles = []  # Red stuff = "rm this part"
        self.brush_mode = "fg"  # Start w/ foreground (fg or bg)
        self.brush_size = 3

    def load_image(self, path):
        """Load img & create a fresh overlay for scribbling"""
        self.image = QImage(path)
        if self.image.isNull():
            return False

        # Fresh transparent overlay - clean slate
        self.overlay = QImage(self.image.size(), QImage.Format.Format_ARGB32)
        self.overlay.fill(Qt.GlobalColor.transparent)

        # Clear
        self.fg_scribbles = []
        self.bg_scribbles = []
        self.update_display()
        return True

    def update_display(self):
        """Redraw everything - img + scribbles on top"""
        if self.image is None:
            return

        # Slap the overlay on top of the image
        combined = QImage(self.image)
        painter = QPainter(combined)
        painter.drawImage(0, 0, self.overlay)
        painter.end()

        # Scale it down to fit the widget nicely
        scaled = combined.scaled(
            self.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation,
        )
        self.setPixmap(QPixmap.fromImage(scaled))

    def get_image_coords(self, widget_pos):
        """
        Convert widget pixel coords → actual image coords
        (Because the img is scaled/centered)
        """
        if self.image is None:
            return None

        pixmap = self.pixmap()
        if pixmap is None:
            return None

        
        offset_x = (self.width() - pixmap.width()) // 2
        offset_y = (self.height() - pixmap.height()) // 2

        # Map click position back to original img coords
        x = int((widget_pos.x() - offset_x) * self.image.width() / pixmap.width())
        y = int((widget_pos.y() - offset_y) * self.image.height() / pixmap.height())

        # bound chk
        if 0 <= x < self.image.width() and 0 <= y < self.image.height():
            return QPoint(x, y)
        return None

    def mousePressEvent(self, event):
        """User clicked - start drawing"""
        if event.button() == Qt.MouseButton.LeftButton:
            self.drawing = True
            self.last_point = self.get_image_coords(event.pos())
            if self.last_point:
                self.draw_point(self.last_point)

    def mouseMoveEvent(self, event):
        """User dragging - keep drawing"""
        if self.drawing and self.last_point is not None:
            current_point = self.get_image_coords(event.pos())
            if current_point:
                self.draw_line(self.last_point, current_point)
                self.last_point = current_point

    def mouseReleaseEvent(self, event):
        """User let go - stop drawing"""
        if event.button() == Qt.MouseButton.LeftButton:
            self.drawing = False
            self.last_point = None

    def draw_point(self, point):
        """Paint a single dot (used when clicking without dragging)"""
        painter = QPainter(self.overlay)
        # Green for fg, red for bg - classic traffic light logic
        pen = QPen(
            QColor(0, 255, 0, 200)
            if self.brush_mode == "fg"
            else QColor(255, 0, 0, 200)
        )
        pen.setWidth(self.brush_size)
        pen.setCapStyle(Qt.PenCapStyle.RoundCap)  # Round brush, not ugly squares
        painter.setPen(pen)
        painter.drawPoint(point)
        painter.end()

        # memtake
        scribbles = self.fg_scribbles if self.brush_mode == "fg" else self.bg_scribbles
        scribbles.append([point.x(), point.y()])

        self.update_display()

    def draw_line(self, start, end):
        """Connect the dots - smooth lines when dragging"""
        painter = QPainter(self.overlay)
        pen = QPen(
            QColor(0, 255, 0, 200)
            if self.brush_mode == "fg"
            else QColor(255, 0, 0, 200)
        )
        pen.setWidth(self.brush_size)
        pen.setCapStyle(Qt.PenCapStyle.RoundCap)
        painter.setPen(pen)
        painter.drawLine(start, end)
        painter.end()

        # Save the endpoint
        scribbles = self.fg_scribbles if self.brush_mode == "fg" else self.bg_scribbles
        scribbles.append([end.x(), end.y()])

        self.update_display()

    def clear_scribbles(self):
        """Nuke everything - fresh start"""
        self.fg_scribbles = []
        self.bg_scribbles = []
        if self.overlay:
            self.overlay.fill(Qt.GlobalColor.transparent)
            self.update_display()


class MainWindow(QMainWindow):
    """
    Main application window
    This is where it all comes together like a beautiful symphony
    (Or a chaotic mess, depending on the day)
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("reImage - Interactive Segmentation")
        self.setGeometry(100, 100, 1200, 800)

        self.image_path = None
        self.temp_dir = tempfile.mkdtemp()  # Throwaway dir for binary files

        # Find the C++ executable (depends on if we're bundled or running as dev)
        if getattr(sys, "frozen", False):
            # PyInstaller exe mode
            base_path = Path(sys._MEIPASS)
            self.cpp_exe = str(base_path / "segment.exe")
        else:
            # Dev mode - just running the script
            self.cpp_exe = str(Path(__file__).parent / "cpp" / "build" / "segment.exe")

        self.init_ui()

    def init_ui(self):
        """Build the UI - buttons, sliders, all that jazz"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QHBoxLayout(central_widget)

        # Left side: The canvas (takes up most of the space)
        self.canvas = ImageCanvas()
        main_layout.addWidget(self.canvas, stretch=3)

        # Right side: Control panel (smaller)
        right_panel = QVBoxLayout()

        # ========== File Operations ==========
        file_group = QGroupBox("File")
        file_layout = QVBoxLayout()

        self.open_btn = QPushButton("📂 Open Image")
        self.open_btn.clicked.connect(self.open_image)
        file_layout.addWidget(self.open_btn)

        self.save_btn = QPushButton("💾 ")
        self.save_btn.clicked.connect(self.save_result)
        self.save_btn.setEnabled(False)  # Disabled until we actually have results
        file_layout.addWidget(self.save_btn)

        file_group.setLayout(file_layout)
        right_panel.addWidget(file_group)

        # ========== Drawing Tools ==========
        draw_group = QGroupBox("Drawing Tools")
        draw_layout = QVBoxLayout()

        # Radio buttons for fg/bg mode
        self.fg_radio = QRadioButton("🟢 Foreground")
        self.fg_radio.setChecked(True)
        self.fg_radio.toggled.connect(lambda: self.set_brush_mode("fg"))
        draw_layout.addWidget(self.fg_radio)

        self.bg_radio = QRadioButton("🔴 Background")
        self.bg_radio.toggled.connect(lambda: self.set_brush_mode("bg"))
        draw_layout.addWidget(self.bg_radio)

        # Brush size slider - because who doesn't like thicc lines
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

        # Clear button -
        self.clear_btn = QPushButton(" Clear Scribbles")
        self.clear_btn.clicked.connect(self.canvas.clear_scribbles)
        draw_layout.addWidget(self.clear_btn)

        draw_group.setLayout(draw_layout)
        right_panel.addWidget(draw_group)

        # ========== Segmentation ==========
        seg_group = QGroupBox("Segmentation")
        seg_layout = QVBoxLayout()

        # run & parse to cpp
        self.segment_btn = QPushButton("⚡ Run Segmentation")
        self.segment_btn.clicked.connect(self.run_segmentation)
        self.segment_btn.setEnabled(False)  # Need an image first, duh
        self.segment_btn.setStyleSheet(
            "QPushButton { font-weight: bold; padding: 10px; }"
        )
        seg_layout.addWidget(self.segment_btn)

        # Progress bar
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

        # Status bar at the bottom -
        self.statusBar().showMessage("DSA PROJECT")

    def open_image(self):
        """Pop open a file dialog - let the user pick their image"""
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open Image",
            "",
            "Image Files (*.png *.jpg *.jpeg *.bmp);;All Files (*)",
        )

        if file_path:
            if self.canvas.load_image(file_path):
                self.image_path = file_path
                self.segment_btn.setEnabled(True)
                self.statusBar().showMessage(f"Loaded: {Path(file_path).name}")
            else:
                QMessageBox.critical(self, "Error", "Failed to load image")

    def set_brush_mode(self, mode):
        """Toggle between fg/bg brush"""
        self.canvas.brush_mode = mode

    def update_brush_size(self, value):
        """Slider changed - update the brush size"""
        self.canvas.brush_size = value
        self.brush_label.setText(str(value))

    def run_segmentation(self):
        """
        The main event - run the segment.exe
        """
        if not self.image_path:
            return

        #? do somethingg
        if not self.canvas.fg_scribbles and not self.canvas.bg_scribbles:
            QMessageBox.warning(
                self,
                "No Scribbles",
                "Please draw some foreground (green) and background (red) scribbles first!",
            )
            return

        self.statusBar().showMessage("Preparing data...")

        try:
            # Get image dimensions
            img = Image.open(self.image_path)
            W, H = img.size

            # Convert img to raw binary format 
            image_bin = os.path.join(self.temp_dir, "image.bin")
            self.export_image_binary(self.image_path, image_bin, W, H)

            # Convert scribbles to seed mask
            seed_bin = os.path.join(self.temp_dir, "seed.bin")
            self.export_seed_mask(seed_bin, W, H)

            output_bin = os.path.join(self.temp_dir, "output_mask.bin")

            # Fire up the worker thread (keeps UI responsive)
            self.segment_btn.setEnabled(False)
            self.progress_bar.setVisible(True)
            self.progress_bar.setRange(0, 0)  # Indeterminate mode - spinny spinner

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
        """Dump image as raw RGB bytes - C++ doesn't do PIL"""
        img = Image.open(image_path).convert("RGB")
        data = np.array(img, dtype=np.uint8)
        data.tofile(output_path)

    def export_seed_mask(self, output_path, W, H):
        """
        Create seed mask from scribbles
        128 = unknown (most of the image)
        255 = foreground seed (green scribbles)
        0 = background seed (red scribbles)
        """
        # Start with everything unknown
        mask = np.full((H, W), 128, dtype=np.uint8)

        # Paint foreground scribbles as 255
        for x, y in self.canvas.fg_scribbles:
            if 0 <= x < W and 0 <= y < H:
                # Draw a filled circle around each point 
                for dy in range(-self.canvas.brush_size, self.canvas.brush_size + 1):
                    for dx in range(
                        -self.canvas.brush_size, self.canvas.brush_size + 1
                    ):
                        if (
                            dx * dx + dy * dy
                            <= self.canvas.brush_size * self.canvas.brush_size
                        ):
                            px, py = x + dx, y + dy
                            if 0 <= px < W and 0 <= py < H:
                                mask[py, px] = 255

        # Paint background scribbles as 0
        for x, y in self.canvas.bg_scribbles:
            if 0 <= x < W and 0 <= y < H:
                for dy in range(-self.canvas.brush_size, self.canvas.brush_size + 1):
                    for dx in range(
                        -self.canvas.brush_size, self.canvas.brush_size + 1
                    ):
                        if (
                            dx * dx + dy * dy
                            <= self.canvas.brush_size * self.canvas.brush_size
                        ):
                            px, py = x + dx, y + dy
                            if 0 <= px < W and 0 <= py < H:
                                mask[py, px] = 0

        mask.tofile(output_path)

    def on_segmentation_finished(self, success, message):
        """Worker thread is done - time to see if it worked or not"""
        self.progress_bar.setVisible(False)
        self.segment_btn.setEnabled(True)

        if success:
            self.statusBar().showMessage(message)
            self.save_btn.setEnabled(True)

            try:
                self.display_result()
                QMessageBox.information(self, "Success", "Segmentation completed!\n")
            except Exception as e:
                # Well, it 
                QMessageBox.warning(
                    self,
                    "Success",
                    f"Segmentation completed but failed to display result:\n{str(e)}",
                )
        else:
            self.statusBar().showMessage("Segmentation failed")
            QMessageBox.critical(self, "Error", message)

    def display_result(self):
        """Show the segmentation result with a nice green overlay"""
        output_mask = os.path.join(self.temp_dir, "output_mask.bin")
        img = Image.open(self.image_path)
        W, H = img.size

        # Load the mask from binary file
        mask = np.fromfile(output_mask, dtype=np.uint8).reshape((H, W))

        # Blend original image with green tint where mask = 1
        img_arr = np.array(img)
        overlay = img_arr.copy()
        # 60% original + 40% green = 
        overlay[mask == 1] = (
            overlay[mask == 1] * 0.6 + np.array([0, 255, 0]) * 0.4
        ).astype(np.uint8)

        # Convert numpy array to QImage 
        height, width, channel = overlay.shape
        bytes_per_line = 3 * width
        overlay_copy = np.ascontiguousarray(overlay)
        q_img = QImage(
            overlay_copy.data,
            width,
            height,
            bytes_per_line,
            QImage.Format.Format_RGB888,
        ).copy()

        # Update the canvas
        self.canvas.image = q_img
        self.canvas.overlay.fill(Qt.GlobalColor.transparent)  # Clear scribbles
        self.canvas.update_display()

    def save_result(self):
        """Save final segmentation with transparent background."""
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Save Result",
            "output.png",
            "PNG Image (*.png);;All Files (*)",
        )
    
        if not file_path:
            return
    
        try:
            output_mask = os.path.join(self.temp_dir, "output_mask.bin")
            img = Image.open(self.image_path).convert("RGB")
            W, H = img.size
    
            mask = np.fromfile(output_mask, dtype=np.uint8).reshape((H, W))
    
            img_arr = np.array(img)
            rgba = np.zeros((H, W, 4), dtype=np.uint8)
            rgba[:, :, :3] = img_arr
            rgba[:, :, 3] = (mask == 1).astype(np.uint8) * 255
    
            result = Image.fromarray(rgba, mode="RGBA")
            result.save(file_path)
    
            self.statusBar().showMessage(f"Saved: {file_path}")
            QMessageBox.information(self, "Saved", f"Result saved to:\n{file_path}")
    
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to save: {str(e)}")


def main():
    
    app = QApplication(sys.argv)
    app.setStyle("Fusion")  #style alias 

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
