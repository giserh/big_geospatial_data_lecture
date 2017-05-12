FROM debian:jessie

# Prepare for HTTPS CRAN repo for R
RUN apt-get update &&  apt-get install -y \
     apt-transport-https \
     ca-certificates 
#     curl \
#     gnupg2 
#     software-properties-common
# Install CRAN repo     
COPY cran.list /etc/apt/sources.list.d
# Add the R maintainer key
RUN apt-key adv --keyserver keys.gnupg.net --recv-key 6212B7B7931C4BB16280BA1306F90DE5381BA480



RUN apt-get update && apt-get install -y \
    	    make git build-essential gcc g++ gdb \ 
	    r-recommended  



# Override home directory of root.
RUN  mkdir /home/bgd && cat /etc/passwd |sed -e "s/\/root/\/home\/bgd/g" > /tmp/passwd && mv /tmp/passwd /etc
COPY profile /home/bgd/.profile

ENV LC_ALL C.UTF-8
ENV DISPLAY :0

CMD ["/bin/su","-l"]
