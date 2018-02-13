#!/bin/sh

echo "#ifndef __ARCHC_TOKENS_H" > archcTokens.h
echo "#define __ARCHC_TOKENS_H" >> archcTokens.h
echo "enum archc_tokens {" >> archcTokens.h

awk '{ if(index($0, "'\''")) { exit } print $0"," }' archc.tokens >> archcTokens.h

echo "};" >> archcTokens.h
echo "#endif" >> archcTokens.h
echo "" >> archcTokens.h
