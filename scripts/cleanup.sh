#!/bin/zsh

i=1
while [ $i -lt 400 ]
do
	j=`printf "%.2d" $i`
	if [ -f results/output_${j}.raw ]
	then
		((i++))
	else
		break
	fi
done

set -e
voxres=800.
if [ $# -gt 0 ]
then
	if [ $1 = '-v' ]
	then
		set -x
	else
		voxres=$1
	fi
fi

config=config.ini


prefix=`grep output_prefix $config |awk -F" ?= ?" '{print $2}'`
size=`grep size $config |awk -F" ?= ?" '{print $2}'`
variation=`grep local_variation config.ini|awk -F" ?= ?" '{print $2}'`
if [ -z $variation ] || [ $variation -eq 0 ]
then
	supp_fname=`grep support_fname config.ini|awk -F" ?= ?" '{print $2}'`
else	
	supp_fname=${prefix}-supp.supp
fi
echo Support: $supp_fname
center=$((size/2))
voxsize=$((voxres / 2. / center))

echo Cleaning to number $j with data from $prefix
cp ${prefix}-pf.raw results/output_pf_${j}.raw
cp ${prefix}-pd.raw results/output_${j}.raw
cp ${prefix}-frecon.raw results/foutput_${j}.raw
cp ${prefix}-log.dat results/log_${j}.dat
cp ${prefix}-prtf.dat results/prtf_${j}.dat
cp ${prefix}-last.raw results/last_${j}.raw
#cp ${prefix}-bg.raw results/bg_${j}.raw
cp ${prefix}-expmag.raw results/expmag_${j}.raw
cp $config results/conf_${j}.ini
echo Generating map
echo ./utils/gen_map results/output_${j}.raw $size $voxsize $voxsize $voxsize $supp_fname
./utils/gen_map results/output_${j}.raw $size $voxsize $voxsize $voxsize $supp_fname
cd data/maps
rm -f output_${j}.mtz
phenix.map_to_structure_factors output_${j}.map.ccp4 box=True output_file_name=output_${j}.mtz
cd -
