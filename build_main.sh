timestamp=$(date +"%Y%m%d_%H%M%S")

# Create logs directory if it doesn't exist
mkdir -p logs

touch logs/build_$timestamp.log
log_file="logs/build_$timestamp.log"

run_dir="${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm_noc/build/scripts/"

# make clean 

echo "building for ${run_dir}"
export JASON_BOARD="iseries-dk"

# cat "${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm/build/scripts/qdb_ofs_pr_afu.txt" > "${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm/fim_platform/build/syn/board/n6001/syn_top/ofs_pr_afu.qsf"

###########################################################################

timestamp=$(date +"%Y%m%d_%H%M%S")
echo "Current timestamp: $timestamp"
make main



