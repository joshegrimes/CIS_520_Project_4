# CIS_520_Project_4

To compile and schedule our code on Beocat:

- Enter each directory (i.e. 3way-pthread, 3way-mpi, or 3-way-openmp)
- Run "sbatch implementation_submit_all.sh" where implementation equals either pthread, mpi, or openmp

- There is no need to run the implementation_script.sh scripts - those are simply run by the implementation_submit_all.sh scripts
- Each executable should be compiled automatically by the scripts

Other notes:
- To run the python script to produce the plots of our analysis data, you may need to be in a virtual machine on Beocat with the proper
  versions of matplotlib, numpy, etc. It took some time for me to figure out the right configuration for it to run properly.

- Some of the larger .out log files were removed from the repo so that it could be pushed to GitHub. They should repopulate once run on Beocat.

