@if {%1}=={} goto noparms
@if {%2}=={} goto noparms

tar -cvf control.tar -C %1%\DEBIAN .
tar -cvf data.tar -C %1%\data .

gzip control.tar
gzip data.tar

ar -r %2% control.tar.gz data.tar.gz
rm -f control.tar.gz data.tar.gz

goto end

:noparms
@echo Usage: 
@echo makedeb.bat debfile directory

:end