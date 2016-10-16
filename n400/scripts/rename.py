from os import listdir
import os
from os.path import isfile, join

if __name__ == "__main__":
    mypath = "."
    files = [f for f in listdir(mypath) if isfile(join(mypath, f))]
    files.remove('pdf.txt')
    files.remove('rename.py')
    with open("pdf.txt") as f:
        filenames = f.readlines()

    # print(filenames)
    mapped = {'{0:0>3}.gif'.format(idx+1):name.split('\n')[0]+'.gif' for idx, name in enumerate(filenames)}
    print(mapped)

    for ix in range(len(files)):
        f = files[ix]

        os.rename(f, mapped[f])
        # print(f)
        # print(mapped[f])
