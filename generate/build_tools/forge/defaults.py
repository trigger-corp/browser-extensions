'Project-wide default values'
import sys
from os import path

FORGE_ROOT = path.dirname(path.dirname(__file__))
CONFIG_FILE = path.join(FORGE_ROOT, 'forge_build.json')
PASSWORD = "your password"

SRC_DIR = 'src'
APP_CONFIG_FILE = path.join(SRC_DIR, 'config.json')
IDENTITY_FILE = path.join(SRC_DIR, 'identity.json')
LOCAL_CONFIG_FILE = 'local_config.json'
TEMPLATE_DIR = '.template'
INSTRUCTIONS_DIR = TEMPLATE_DIR
