cmd_/home/ilya/lab2/Module.symvers := sed 's/ko$$/o/' /home/ilya/lab2/modules.order | scripts/mod/modpost     -o /home/ilya/lab2/Module.symvers -e -i Module.symvers   -T -
