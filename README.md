# HPRDP
High performance reverse SSH RDP solution (not really RDP, more like teamviewer..)

## Infrastructure 

### Reverse SSH configuration
If you need to connect between two NAT networks without opening ports on either side (like t.w. does)

1. On a public server you own, configure ssh gateway ports by adding `GatewayPorts clientspecified` to `sshd_config`
2. On the remote computer you want to connect to, enable ssh and create a key to be used to connect without password:
```
systemctl enable --now ssh
ssh-keygen -t rsa -f ~/.ssh/reverse-key
ssh-copy-id -i ~/.ssh/reverse-key <user@server>
```
3. Now create a tunnel so you can forward all ssh requests (on port 8080 for example) to the ssh service running on your remote machine: 
```
ssh -i ~/.ssh/reverse-key -Ng -R *:8080:localhost:22 user@server
```


## USAGE
In the following section we will refer to `IP` as: 
- `Your public server IP` if you did the `Reverse SSH configuration`
- `Your remove computer IP` if you open ports `12345` and `12346` on your NAT network. 
In such case `-p 8080` must be omitted as we're connecting to standard port 22.
 
1. Connect to the remote computer and forward local requests on ports `12345` and `12346` to localhost:
```
ssh -R 12345:127.0.0.1:12345 -R 12346:127.0.0.1:12346 user@IP -p 8080
# This will also yield a shell you can use on step 3.
```
2. Launch `HPRDP-x86_64.AppImage` on your local computer.
3. `export DISPLAY=:0.0` and launch `HPRDP-KVM-4f4e79b-x86_64.AppImage` on your remote computer
