import shutil
from glob import glob
import os
import random

if __name__ == '__main__':
    src_dir = os.curdir
    dest_dir_name = 'experiment'
    filenames = []

    # obtain images in current cirectory
    for ext in '*.jpg', '*.png':
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
        idx=idx+1, filename=filename) for idx, filename in enumerate(filenames)}

    # copy files to new folder
    for old_filename, new_filename in new_filenames.items():
        # copy
        src_file = os.path.join(src_dir, old_filename)
        shutil.copy(src_file, dest_dir)

        # rename
        src_file = os.path.join(dest_dir, old_filename)
        dest_file = os.path.join(dest_dir, new_filename)
        os.rename(src_file, dest_file)
