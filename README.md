# HPRDP
High performance reverse SSH RDP solution (not really RDP, more like teamviewer..)

## Infrastructure 

### Reverse SSH configuration
If you need to connect between two NAT networks without opening ports on either side (like t.w. does)

1. On a public server you own, configure ssh gateway ports by adding `GatewayPorts clientspecified` to `sshd_config`
2. On the remote computer you want to connect to, enable ssh and *optionally* create a key to be used to connect without password:
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
- `Your remote computer IP` if you can open port `22` on the remote computer NAT network. 
In such case `-p 8080` must be omitted unless you decide to open port `8080` to `22` in your router (suggested).
 
1. Connect to the remote computer and forward local requests on ports `12345` and `12346` to localhost:
```
ssh -R 12345:127.0.0.1:12345 -R 12346:127.0.0.1:12346 user@IP -p 8080
# This will also yield a shell you can use on step 3.
```
2. Launch `HPRDP-x86_64.AppImage` on your local computer.
3. Launch `HPRDP-KVM-4f4e79b-x86_64.AppImage` on your remote computer 


That's it!

You should now be able to see and control the remote screen.

## Configuration
Configuration is kept minimal and you get (arguably) reasonable default settings out of the box. 

### Network configuration
Everything is binding on `localhost` to let the user tunnel the traffic the way he/she wants. 
In order to maximize flexibility and keep the code simple, the application should not care about network configuration and encryption.
Plese note that encryption is not provided so SSH (or equivalent encrypted tunneling such as VPN) is highly recommended.

### FFMPEG streaming configuration
On first launch, the application writes a default ffmpeg command line under `~/.config/hprdpvideo.sh`:
```
echo $BASHPID > /tmp/ffmpeg.pid; ffmpeg -f x11grab -draw_mouse 0 -s $screensize -framerate 30 -i $DISPLAY  -c:v libx264 -preset medium -profile high -pix_fmt yuv420p -tune zerolatency -b:v 2000K -minrate 2000K -maxrate 2000K -bufsize 512k -f mpegts tcp://127.0.0.1:12345
```
You are free to change anything at your will, but be sure to keep the `echo $BASHPID > /tmp/ffmpeg.pid;` prefix as it's needed for terminating ffmpeg via UI dialog.
