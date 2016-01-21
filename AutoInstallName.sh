#!/bin/bash

for arg in "$@"
do
#install_name_tool -id @executable_path/../Frameworks/$arg.framework/Versions/5/$arg ConceptClient.app/Contents/Frameworks/$arg.framework/Versions/5/$arg
echo "/Users/eduard/Qt/5.3/clang_64/lib/$arg.framework/Versions/5.0/$arg:"
    install_name_tool -change "/Users/eduard/Qt/5.3/clang_64/lib/$arg.framework/Versions/5.0/$arg" "\@executable_path/../Frameworks/$arg.framework/Versions/5.0/$arg" ConceptClient.app/Contents/MacOS/components/scintilla.cl
done
otool -L ConceptClient.app/Contents/MacOS/components/scintilla.cl