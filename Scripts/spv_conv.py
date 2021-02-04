"""
@file spv_conv.py
@brief すべてのglslおよびhlslをspir-vにコンパイルします。
"""


import os
import logging
from pathlib import Path
import subprocess
# import argparse


class Compiler(object):
    def __init__(self, logger=None):
        scripts_path = Path('.').resolve()
        working_directory_path = scripts_path.parent
        self.SHADERS_PATH = working_directory_path.joinpath(
            'Assets', 'Shaders')

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

        # シェーダーステージの対応
        self.EXT2STAGE = {'vs': 'vert', 'fs': 'frag',
                          'gs': 'geom', 'tc': 'tesc',
                          'te': 'tese', 'cs': 'comp'}

    # GLSLもしくはHLSLのすべてのファイルをSPIR-Vへコンパイルします。
    def compiles(self, lang):
        self.logger.info(lang + ' - start compile.')

        for root, dirs, files in os.walk(self.SHADERS_PATH.joinpath(lang)):
            [self.compile(f, root) for f in files if f.endswith('lsl')]

        self.logger.info(lang + ' - finished compile.')

    # 指定したシェーダーファイルをSPIR-Vへコンパイルします。
    def compile(self, shader, shader_dir):
        stage = self.EXT2STAGE[shader.split('.')[1]]
        src = str(Path(shader_dir).joinpath(shader))
        dst = Path(shader_dir).parent.joinpath(
            'SPIR-V', Path(shader_dir).name,
            Path(shader).with_suffix('.spv').name)
        self.logger.debug('src - {}'.format(src))
        self.logger.debug('dst - {}'.format(dst))

        # glslcを使います。
        # 詳しくは https://github.com/google/shaderc/tree/main/glslc を参照ください。
        self.logger.info('Start compile: {}'.format(src))
        cp = subprocess.run(
            ['glslc', '-fshader-stage={}'.format(stage), src, '-o', dst])
        if cp.returncode != 0:
            self.logger.error('Failed to compile: {}'.format(src))
        else:
            self.logger.info('Finished compile: {}'.format(src))


if __name__ == '__main__':
    compiler = Compiler()

    compiler.compiles('GLSL')
    compiler.compiles('HLSL')
