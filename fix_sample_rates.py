import os
import pathlib, tinytag, subprocess
import shutil

for wav in pathlib.Path('workdir/instruments').rglob('*.wav'):
    samplerate = tinytag.TinyTag.get(wav).samplerate
    if samplerate != 48000:
        print('F i x i n g', wav)
        tmp = str(wav) + '.tmp.wav'
        ret = subprocess.call(['ffmpeg', '-y', '-i', wav, '-ac', '1', '-ar', '48000', tmp], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if ret == 0:
            shutil.copy2(tmp, wav)
        else:
            print('error')
        os.remove(tmp)
    else:
        print('\t- OK', wav, samplerate)