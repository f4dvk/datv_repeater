#!/bin/bash

Nb=0

wile true;do
  multimon
  sleep 1
  let Nb=Nb+1
  dt=$(date '+%d/%m/%Y %H:%M:%S')
  echo $dt Nombre de redémarrage: $Nb
done
