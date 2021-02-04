"""
@file texconv.py
@brief すべてのpngファイルをktxおよびdds(DXT5)ファイルに変換します。
"""


import os
import logging
from pathlib import Path
import subprocess
# import argparse

class Converter(object):
    def __init__(self, logger=None):
        scripts_path = Path('.').resolve()
        working_directory_path = scripts_path.parent
        self.TEXTURES_PATH = working_directory_path.joinpath(
            'Assets', 'Textures')

        # ロガーの設定
        self.LOGGER_FORMAT = '%(asctime)s - %(levelname)s: %(message)s'
        if logger is None:
            self.logger = logging.getLogger(__name__)
            ch = logging.StreamHandler()
            formatter = logging.Formatter(self.LOGGER_FORMAT)
            ch.setFormatter(formatter)
            self.logger.addHandler(ch)
            self.logger.setLevel(logging.INFO)
        else:
            self.logger = logger

    def png2ktx(self):
        self.logger.info('png -> ktx: start convert.')

        for root, dirs, files in os.walk(self.TEXTURES_PATH.joinpath('png')):
            [self.to_ktx(f, root) for f in files if f.endswith('png')]
            
        self.logger.info('png -> ktx: finished convert.')
    
    def to_ktx(self, png, target_dir):
        src = str(Path(target_dir).joinpath(png))
        dst = Path(target_dir).parent.parent.joinpath(
            'ktx', Path(target_dir).name,
            Path(png).with_suffix('.ktx').name)
        self.logger.debug('src - {}'.format(src))
        self.logger.debug('dst - {}'.format(dst))

        # toktxを使用します。
        # 詳しくは https://github.com/KhronosGroup/KTX-Software
        self.logger.info('Start convert: {}'.format(src))
        cp = subprocess.run(
            ['toktx', '--2d', '--genmipmap', dst, src])
        if cp.returncode != 0:
            self.logger.error('Failed to convert: {}'.format(src))
        else:
            self.logger.info('Finished convert: {}'.format(src))

    def png2dds(self):
        self.logger.info('png -> dds(dxt5): start convert.')

        for root, dirs, files in os.walk(self.TEXTURES_PATH.joinpath('png')):
            [self.to_dds(f, root) for f in files if f.endswith('png')]
            
        self.logger.info('png -> dds(dxt5): finished convert.')

    def to_dds(self, png, target_dir):
        src = str(Path(target_dir).joinpath(png))
        dst = Path(target_dir).parent.parent.joinpath(
            'dds', 'dxt5', Path(target_dir).name,
            Path(png).with_suffix('.dds').name)
        self.logger.debug('src - {}'.format(src))
        self.logger.debug('dst - {}'.format(dst))

        # image magickを使用します。
        self.logger.info('Start convert: {}'.format(src))
        cp = subprocess.run(
            ['convert', '-format', 'dds', '-define', 'dds:compression=dxt5', src, dst])
        if cp.returncode != 0:
            self.logger.error('Failed to convert: {}'.format(src))
        else:
            self.logger.info('Finished convert: {}'.format(src))

if __name__ == '__main__':
    converter = Converter()
    #converter.png2ktx()
    converter.png2dds()
