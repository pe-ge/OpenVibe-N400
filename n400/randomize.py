import shutil
from glob import glob
import os
import random
from PIL import Image, ImageDraw, ImageFont

if __name__ == '__main__':
    src_dir = os.curdir
    dest_dir_name = 'experiment'
    filenames = []

    # text image params
    W, H = (400, 400)
    font = ImageFont.truetype("arial.ttf", 40)

    # obtain images in current cirectory
    for ext in '*.jpg', '*.png', '*.gif':
        filenames.extend(glob(ext))

    # create new folder (check whether old one exists)
    count = 1
    dest_dir = os.path.join(src_dir, dest_dir_name + str(count))
    while os.path.exists(dest_dir):
        count += 1
        dest_dir = os.path.join(src_dir, dest_dir_name + str(count))
    os.makedirs(dest_dir)

    # shuffle filenames
    random.shuffle(filenames)

    # append idxs in ascending order to filenames
    new_filenames = {filename: '{idx:03}_{filename}'.format(
        idx=(2*idx + 1), filename=filename) for idx, filename in enumerate(filenames)}

    # copy files to new folder
    for old_filename, new_filename in new_filenames.items():
        # copy
        src_file = os.path.join(src_dir, old_filename)
        shutil.copy(src_file, dest_dir)

        # rename
        src_file = os.path.join(dest_dir, old_filename)
        dest_file = os.path.join(dest_dir, new_filename)
        os.rename(src_file, dest_file)

        # create image with text
        image = Image.new("RGBA", (W, H))
        draw = ImageDraw.Draw(image)

        text = old_filename.split('.')[0]  # remove extension
        w, h = draw.textsize(text)
        draw.text(((W-w)/2, (H-h)/2), text, fill="black", font=font)

        prefix, sufix = new_filename.split('_')
        prefix = int(prefix) + 1
        new_filename = '{prefix:03}_{filename}'.format(
            prefix=prefix, filename=sufix)
        dest_file = os.path.join(dest_dir, new_filename)
        image.save(dest_file, "PNG")
