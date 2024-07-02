#!/bin/bash
APP_NAME=mybee-qt
TS_PATH=translations/$APP_NAME-ru_RU.ts
QT_PATH=~/Qt5.15.10/clang_64/bin
$QT_PATH/lupdate $APP_NAME.pro -ts $TS_PATH
#$QT_PATH/lrelease $TS_PATH
