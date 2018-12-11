#! /bin/bash

echo "[set COS]"
pqos -e "llc:1=0x0000f;llc:2=0x000f0;llc:3=0x00f00;llc:4=0x0f000"

echo "[bind core to COS]"
pqos -a "core:1=40;core:2=42;core:3=44;core:4=46"
