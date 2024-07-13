#!/bin/sh

cat <<EOD > tool/version.h
#pragma once
#define VERSION "${1}"
EOD
