echo '\newpage{}' > udp.verbatim
echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/include/prot/udp.h  >> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
echo '\newpage{}' >> udp.verbatim

echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/protocols/udp/udp_internal.h >> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
echo '\newpage{}' >> udp.verbatim

echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/protocols/udp/udp.c >> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
echo '\newpage{}' >> udp.verbatim

echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/protocols/udp/udp_port.h >> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
echo '\newpage{}' >> udp.verbatim

echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/protocols/udp/udp_port.c >> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
echo '\newpage{}' >> udp.verbatim

echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/protocols/util/port_mgr.h>> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
echo '\newpage{}' >> udp.verbatim

echo '\begin{verbatim}' >> udp.verbatim
expand < ../../merge/protocols/util/port_mgr.c>> udp.verbatim
echo '\end{verbatim}' >> udp.verbatim
