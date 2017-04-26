import glob
import os
import mtranslate
import shutil

pictures = [filename for filename in glob.iglob('good_pictures_gray/**/*.*', recursive=True)]
for picture in pictures:
    path, name = os.path.split(picture)
    dot_idx = name.rfind('.')
    ext = name[dot_idx + 1:]
    name = name[:dot_idx]
    translated_name = mtranslate.translate(name, to_language="sk", from_language="en").lower()
    # print(translated_name)
    new_picture = os.path.join(path, translated_name + '.' + ext)
    if not os.path.exists(new_picture):
        os.rename(picture, new_picture)
    else:
        print(picture, new_picture)
