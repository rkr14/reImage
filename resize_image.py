from PIL import Image

img = Image.open("input_new.jpg")
img_small = img.resize((800, 500), Image.LANCZOS)  # choose any size
img_small.save("input_small.jpg")
