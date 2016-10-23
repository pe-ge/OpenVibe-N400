import shutil
import glob
import os
import random
from PIL import Image, ImageDraw, ImageFont

N = 100  # number of pairs


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


def load_dataset():
    return [filename for filename in glob.iglob('dataset/**/*.*', recursive=True)]


def create_experiment_directories(src_dir, dest_dir_name):
    count = 1
    dest_dir = os.path.join(src_dir, dest_dir_name + str(count))
    while os.path.exists(dest_dir):
        count += 1
        dest_dir = os.path.join(src_dir, dest_dir_name + str(count))

    dest_dir1 = os.path.join(dest_dir, "1")
    dest_dir2 = os.path.join(dest_dir, "2")
    os.makedirs(dest_dir1)
    os.makedirs(dest_dir2)
    return dest_dir1, dest_dir2


def save_cross(W, H, filename, dest_dir):
    image = Image.new("RGBA", (W, H))
    draw = ImageDraw.Draw(image)
    border = 100
    draw.line([W/2, border, W/2, H - border],  fill="black", width=10)
    draw.line([border, H/2, W - border, H/2],  fill="black", width=10)
    image.save(os.path.join(dest_dir, filename), "PNG")


def process_dataset(dataset, dest_dir):
    save_cross(W, H, "000_cross.png", dest_dir)
    # append idxs in ascending order to filenames
    new_filenames = {}
    for idx, filename in enumerate(dataset):
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
        text = text.lower()
        text = text.replace('_', ' ')

        w, h = font.getsize(text)
        draw.text(((W-w)/2, (H-h)/2), text, fill="black", font=font)

        new_filename = os.path.split(new_filename)[1].split('_')
        prefix = int(new_filename[0]) + 1
        # remap prefix
        prefix = filenames_map.get(prefix, prefix)

        new_filename = '{prefix:03}_{filename}'.format(
                prefix=prefix, filename='_'.join(new_filename[1:]))
        dest_file = os.path.join(dest_dir, new_filename)
        image.save(dest_file, "PNG")

if __name__ == '__main__':
    src_dir = os.curdir
    dest_dir_name = 'experiment'
    W, H = (600, 600)
    font = ImageFont.truetype("verdana.ttf", 50)

    full_dataset = load_dataset()
    random.shuffle(full_dataset)
    dataset1 = full_dataset[0:N]
    dataset2 = full_dataset[N:2*N]

    dest_dir1, dest_dir2 = create_experiment_directories(src_dir, dest_dir_name)

    process_dataset(dataset1, dest_dir1)
    process_dataset(dataset2, dest_dir2)

    # save file order to txt
    with open(os.path.split(dest_dir1)[0] + '.txt', 'w') as f:
        for filename in sorted(glob.glob(os.path.join(dest_dir1, '*.*'))):
            f.write('{}\n'.format(filename))
        for filename in sorted(glob.glob(os.path.join(dest_dir2, '*.*'))):
            f.write('{}\n'.format(filename))
