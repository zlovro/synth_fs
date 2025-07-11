import pathlib

instrument = input('instrument: ')
amount = int(input('amount: '))

for wav in pathlib.Path('instruments').joinpath(instrument).glob('*.wav'):
    parts = wav.name.split('.')[0].split('_')
    semitone, velocity = map(int, parts)
    semitone += amount
    wav.rename(wav.with_name(f'{semitone}_{velocity}.wav'))