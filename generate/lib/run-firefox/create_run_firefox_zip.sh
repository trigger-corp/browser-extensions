#!/bin/sh

find . -name '*.pyc' | xargs rm
zip -r run-firefox.zip cuddlefish mozrunner simplejson firefox_runner.py
