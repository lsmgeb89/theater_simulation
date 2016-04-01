#!/bin/bash
#

readonly NAME="cs5348_project_2"
readonly RELEASE_FOLDER="${HOME}/${NAME}"
readonly RELEASE_ZIP="${HOME}/${NAME}.zip"

# delete previous release zip
if [ -f "$RELEASE_ZIP" ]; then
  rm "$RELEASE_ZIP"
fi

mkdir -p "$RELEASE_FOLDER"/src
# copy source files
cp theater_simulation/*.cc theater_simulation/*.h theater_simulation/CMakeLists.txt "$RELEASE_FOLDER"/src
# copy readme.txt
cp theater_simulation/readme.txt "$RELEASE_FOLDER"
# copy testcase
cp movies.txt "$RELEASE_FOLDER"
# compile summary.tex
pushd summary
pdflatex -output-directory="$RELEASE_FOLDER" summary.tex
popd
# compile design.tex
pushd design
pdflatex -output-directory="$RELEASE_FOLDER" design.tex
popd
# clean auxiliary files
pushd "$RELEASE_FOLDER"
rm *.aux *.log
popd
# package all files
pushd "${HOME}"
zip -r "$RELEASE_ZIP" "$NAME"/*
chmod 777 "$RELEASE_ZIP"
popd

# delete release folder
if [ -d "$RELEASE_FOLDER" ]; then
  rm -rf "$RELEASE_FOLDER"
fi
