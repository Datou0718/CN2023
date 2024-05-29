FROM ubuntu:20.04

# install packages
RUN apt update
RUN yes | unminimize
RUN apt install -y tini iproute2 iputils-ping net-tools netcat
RUN apt install -y openssh-server sudo vim grep gawk rsync tmux man manpages manpages-dev manpages-posix manpages-posix-dev diffutils
RUN apt install -y gcc g++ gdb make yasm nasm tcpdump libcapstone-dev python3
RUN apt install -y libc6-dbg dpkg-dev
RUN apt install -y curl git zsh
RUN apt install -y apache2-utils
RUN apt install -y ffmpeg

# /var/run/sshd: required on ubuntu
RUN mkdir /var/run/sshd
FROM ubuntu:20.04

# install packages
RUN apt update
RUN yes | unminimize
RUN apt install -y tini iproute2 iputils-ping net-tools netcat
RUN apt install -y openssh-server sudo vim grep gawk rsync tmux man manpages manpages-dev manpages-posix manpages-posix-dev diffutils
RUN apt install -y gcc g++ gdb make yasm nasm tcpdump libcapstone-dev python3
RUN apt install -y libc6-dbg dpkg-dev
RUN apt install -y curl git zsh
RUN apt install -y apache2-utils
RUN apt install -y ffmpeg

# /var/run/sshd: required on ubuntu
RUN mkdir /var/run/sshd

# locale
RUN apt install -y locales
ENV LANGUAGE en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN /usr/sbin/locale-gen

# gen ssh-keys, allow empty password
#RUN ssh-keygen -t dsa -f /etc/ssh/ssh_host_dsa_key
#RUN ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key
RUN echo 'PermitEmptyPasswords yes' >> /etc/ssh/sshd_config
RUN sed -i 's/nullok_secure/nullok/' /etc/pam.d/common-auth

# add user/group, empty password, allow sudo
RUN groupadd -g 1000 cn
RUN useradd --uid 1000 --gid 1000 --groups root,sudo,adm,users --create-home --password '' --shell /bin/bash cn
RUN echo '%sudo ALL=(ALL) ALL' >> /etc/sudoers
RUN mkdir -p /home/cn/hw2

# run the service
EXPOSE 22
ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/usr/sbin/sshd", "-D"]
# locale
RUN apt install -y locales
ENV LANGUAGE en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN /usr/sbin/locale-gen

# gen ssh-keys, allow empty password
#RUN ssh-keygen -t dsa -f /etc/ssh/ssh_host_dsa_key
#RUN ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key
RUN echo 'PermitEmptyPasswords yes' >> /etc/ssh/sshd_config
RUN sed -i 's/nullok_secure/nullok/' /etc/pam.d/common-auth

# add user/group, empty password, allow sudo
RUN groupadd -g 1000 cn
RUN useradd --uid 1000 --gid 1000 --groups root,sudo,adm,users --create-home --password '' --shell /bin/bash cn
RUN echo '%sudo ALL=(ALL) ALL' >> /etc/sudoers
RUN mkdir -p /home/cn/hw2

# run the service
EXPOSE 22
ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/usr/sbin/sshd", "-D"]
