import shutil
import glob
import os
import random
from PIL import Image, ImageDraw, ImageFont


def create_random_dict(max_items):
    def equals(rand_keys, rand_values):
        return any([elem[0] == elem[1] for elem in zip(rand_keys, rand_values)])

    rand_keys = [2*(i+1) for i in range(max_items)]
    random.shuffle(rand_keys)
    rand_keys = rand_keys[::2]  # we need only half

    rand_values = rand_keys[:]
    random.shuffle(rand_values)

    # we dont want any key to be equal with its value
    while(equals(rand_keys, rand_values)):
        random.shuffle(rand_values)

    return dict(zip(rand_keys, rand_values))

if __name__ == '__main__':
    src_dir = os.curdir
    dest_dir_name = 'experiment'
    filenames = []

    # iterate recursively over images inside dataset/
    for filename in glob.iglob('dataset/**/*.*', recursive=True):
        filenames.append(filename)

    # text image params
    W, H = (400, 400)
    font = ImageFont.truetype("verdana.ttf", 40)

    # create new folder (check whether old one exists)
    count = 1
    dest_dir = os.path.join(src_dir, dest_dir_name + str(count))
    while os.path.exists(dest_dir):
        count += 1
        dest_dir = os.path.join(src_dir, dest_dir_name + str(count))
    os.makedirs(dest_dir)

    # draw cross into first image
    image = Image.new("RGBA", (W, H))
    draw = ImageDraw.Draw(image)
    draw.line([W/2, 0, W/2, H],  fill="black", width=10)
    draw.line([0, H/2, W, H/2],  fill="black", width=10)
    image.save(os.path.join(dest_dir, "000_cross.png"), "PNG")

    # shuffle filenames
    random.shuffle(filenames)

    # append idxs in ascending order to filenames
    new_filenames = {}
    for idx, filename in enumerate(filenames):
        directory, img_name = os.path.split(filename)
        new_img_name = '{:03}_{}'.format(2 * idx + 1, img_name)
        new_filename = os.path.join(dest_dir, new_img_name)
        new_filenames[filename] = new_filename

    filenames_map = create_random_dict(len(new_filenames))

    # copy files to new folder
    for old_filename, new_filename in new_filenames.items():
        # copy
        src_file = os.path.join(src_dir, old_filename)
        shutil.copy(src_file, dest_dir)

        # rename
        old_filename = os.path.split(old_filename)[1]
        old_filename = os.path.join(dest_dir, old_filename)
        os.rename(old_filename, new_filename)

        # create image with text
        image = Image.new("RGBA", (W, H))
        draw = ImageDraw.Draw(image)

        # prepare text
        text = os.path.split(old_filename)[1].split('.')[0]  # remove extension
        text = text.capitalize()
        text = text.replace('_', ' ')

        w, h = font.getsize(text)
        draw.text(((W-w)/2, (H-h)/2), text.capitalize(), fill="black", font=font)

        new_filename = os.path.split(new_filename)[1].split('_')
        prefix = int(new_filename[0]) + 1
        # remap prefix
        prefix = filenames_map.get(prefix, prefix)

        new_filename = '{prefix:03}_{filename}'.format(
                prefix=prefix, filename='_'.join(new_filename[1:]))
        dest_file = os.path.join(dest_dir, new_filename)
        image.save(dest_file, "PNG")

    # save file order to txt
    with open(dest_dir + '.txt', 'w') as f:
        for filename in sorted(glob.glob(os.path.join(dest_dir, '*.*'))):
            f.write('{}\n'.format(os.path.split(filename)[-1]))
