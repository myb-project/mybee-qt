#!/bin/bash
APP_NAME=mybee-qt
TS_PATH=src/translations/$APP_NAME-ru_RU.ts
QT_PATH=/usr/lib/qt6/bin
$QT_PATH/lupdate $APP_NAME.pro -ts $TS_PATH
#$QT_PATH/lrelease $TS_PATH
