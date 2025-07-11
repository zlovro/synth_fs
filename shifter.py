import os
import pathlib
import subprocess
import sys
import time

KEY_FIRST = 24
KEY_COUNT = 61

d = pathlib.Path(sys.argv[1])
print(d)

def main():
    if not d.is_dir():
        return

    # if 'config.json' not in [x.name for x in d.glob('*.json')]:
    #     return
    def get_wav(tone, velocity):
        return d.joinpath(f'{tone}_{velocity}.wav')

    data = {}
    for wav in d.glob('*.wav'):
        parts = wav.name.split('.')[0].split('_')
        semitone, velocity = map(int, parts)
        if semitone not in data.keys():
            data[semitone] = []

        data[semitone].append(velocity)


    def closest_tone(tone):
        return min(data.keys(), key = lambda x: abs(x - tone))


    for k in range(KEY_FIRST, KEY_FIRST + KEY_COUNT):
        closest = closest_tone(k)
        if k == closest:
            continue

        print('\tShifting', closest, 'to', k, end = '\r')

        for velocity in data[closest]:
            code = subprocess.check_call(['rubberband-r3', '-t', '1.0', '-p', str(k - closest), get_wav(closest, velocity), get_wav(k, velocity).with_suffix('.wav')], stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)
            if code != 0:
                print('shit', code)


    print()

try:
    main()
except Exception as e:
    print(e)