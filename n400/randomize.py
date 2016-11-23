import shutil
import glob
import os
import random
from PIL import Image, ImageDraw, ImageFont

N = 60  # number of pairs
dest_dir_name = 'experiment'
W, H = (600, 600)  # dimension of word image
font = ImageFont.truetype("verdana.ttf", 50)


def load_dataset():
    return [filename for filename in glob.iglob('dataset/**/*.*', recursive=True)]


def create_experiment_directories(src_dir):
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


def save_text(W, H, path, text):
    image = Image.new("RGBA", (W, H))
    draw = ImageDraw.Draw(image)
    draw.rectangle([(0,0),image.size], fill = (200,200,200))
    w, h = font.getsize(text)
    draw.text(((W-w)/2, (H-h)/2), text.lower(), fill="black", font=font)
    image.save(path, "PNG")


def save_cross(W, H, path):
    image = Image.new("RGBA", (W, H))
    draw = ImageDraw.Draw(image)
    draw.rectangle([(0,0),image.size], fill = (200,200,200))
    border = 100
    draw.line([W/2, border, W/2, H - border],  fill="black", width=10)
    draw.line([border, H/2, W - border, H/2],  fill="black", width=10)
    image.save(path, "PNG")


def prepare_text(path):
    text = os.path.split(path)[1].split('.')[0]  # remove extension
    return text.replace('_', ' ')


def process_dataset(dataset, dest_dir):
    save_cross(W, H, os.path.join(dest_dir, "000_cross.png"))
    # append idxs in ascending order to filenames
    new_paths = {}
    for idx, path in enumerate(dataset[0:2*N]):
        directory, img_name = os.path.split(path)
        new_img_name = '{:03}_{}'.format(2 * idx + 1, img_name)
        new_path = os.path.join(dest_dir, new_img_name)
        new_paths[path] = new_path

    # idxs of picturesthat mismatch word
    mismatched = [False] * N + [True] * N
    random.shuffle(mismatched)
    m_idx = 0  # iterating over mismatched list
    d_idx = 0  # iterating over dataset[2*N:] list

    # copy files to new folder
    for old_filename, new_filename in new_paths.items():
        # copy
        src_file = os.path.join(os.curdir, old_filename)
        shutil.copy(src_file, dest_dir)

        # rename
        old_filename = os.path.split(old_filename)[1]
        old_filename = os.path.join(dest_dir, old_filename)
        os.rename(old_filename, new_filename)

        if mismatched[m_idx]:
            try:
                text = prepare_text(dataset[2*N + d_idx])
            except IndexError:
                print('NOT ENOUGH PICTURES')
                raise
            d_idx += 1
        else:
            text = prepare_text(old_filename)
        m_idx += 1

        new_filename = os.path.split(new_filename)[1].split('_')
        prefix = int(new_filename[0]) + 1

        new_filename = '{prefix:03}_{filename}.png'.format(
                prefix=prefix, filename=text)
        dest_file = os.path.join(dest_dir, new_filename)
        save_text(W, H, dest_file, text)

if __name__ == '__main__':

    full_dataset = load_dataset()
    random.shuffle(full_dataset)
    dataset1 = full_dataset[0:3*N]
    dataset2 = full_dataset[3*N:6*N]

    dest_dir1, dest_dir2 = create_experiment_directories(os.curdir)

    process_dataset(dataset1, dest_dir1)
    process_dataset(dataset2, dest_dir2)

    # save file order to txt
    with open(os.path.split(dest_dir1)[0] + '.txt', 'w') as f:
        for filename in sorted(glob.glob(os.path.join(dest_dir1, '*.*'))):
            f.write('{}\n'.format(filename))
        for filename in sorted(glob.glob(os.path.join(dest_dir2, '*.*'))):
            f.write('{}\n'.format(filename))
