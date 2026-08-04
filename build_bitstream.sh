timestamp=$(date +"%Y%m%d_%H%M%S")

# Create logs directory if it doesn't exist
mkdir -p logs

touch logs/build_$timestamp.log
log_file="logs/build_$timestamp.log"

run_dir="${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm_noc/build/scripts/"

make clean 

echo "building for ${run_dir}"

# timestamp=$(date +"%Y%m%d_%H%M%S")
# echo "Current timestamp: $timestamp"
# echo "starting src"
# cat $run_dir"run_compile_all.txt" > $run_dir"run.sh"
# make source >> $log_file 2>&1

# timestamp=$(date +"%Y%m%d_%H%M%S")
# echo "Current timestamp: $timestamp"
# echo "starting slot0"
# cat $run_dir"run1.txt" > $run_dir"run.sh"
# make s0 >> $log_file 2>&1

# timestamp=$(date +"%Y%m%d_%H%M%S")
# echo "Current timestamp: $timestamp"
# echo "starting slot1"
# cat $run_dir"run2.txt" > $run_dir"run.sh"
# make s1 >> $log_file 2>&1

# cat "${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm_noc/build/scripts/build_3_ofs_pr_afu.txt" > "${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm_noc/fim_platform/build/syn/board/iseries-dk/syn_top/ofs_pr_afu.qsf"
timestamp=$(date +"%Y%m%d_%H%M%S")
echo "Current timestamp: $timestamp"
echo "starting slot2"
cat $run_dir"run2.txt" > $run_dir"run.sh"
make s2_src >> $log_file 2>&1

# cat "${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm_noc/build/scripts/build_4_ofs_pr_afu.txt" > "${OFS_ASP_ROOT}/hardware/ofs_iseries-dk_usm_noc/fim_platform/build/syn/board/iseries-dk/syn_top/ofs_pr_afu.qsf"
timestamp=$(date +"%Y%m%d_%H%M%S")
echo "Current timestamp: $timestamp"
echo "starting slot3"
cat $run_dir"run3.txt" > $run_dir"run.sh"
make s3_sink >> $log_file 2>&1