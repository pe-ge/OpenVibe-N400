import glob
from PIL import Image

old_color = (255, 255, 255)
new_color = (170, 170, 170)

if __name__ == '__main__':
    picture_paths = [filename for filename in glob.iglob('replace/**/*.*', recursive=True)]
    for path in picture_paths:
        img = Image.open(path)
        pixdata = img.load()
        width, height = img.size
        for x in range(width):
            for y in range(height):
                if pixdata[x, y] == old_color:
                    pixdata[x, y] = new_color
        img.save(path + 'a', 'png')
