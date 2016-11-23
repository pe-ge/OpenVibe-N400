import glob
import os
from PIL import Image

old_color = 255
new_color = 200
tolerance = 50

def within_tolerance(color, new_color=new_color):
    result = True
    for col in color:
        result &= col >= old_color - tolerance
    return result


if __name__ == '__main__':
    picture_paths = [filename for filename in glob.iglob('replace/**/*.*', recursive=True)]
    if not os.path.exists('replaced'):
        os.makedirs('replaced')
    for path in picture_paths:
        img = Image.open(path)
        img = img.convert('RGB')
        pixdata = img.load()
        width, height = img.size
        for x in range(width):
            for y in range(height):
                if within_tolerance(pixdata[x, y]):
                    pixdata[x, y] = (new_color, ) * 3

        dest_dir = os.path.join('replaced', os.path.split(path)[0])
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
        img.save(os.path.join('replaced', path))
