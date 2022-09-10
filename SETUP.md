# Setup

## Setup Windows 11

We're going to enable Internet Connection Sharing on the WiFi so that the Raspberry Pi can access the internet.
Open Control Panel (not Settings) -> Network and Sharing Centre -> Change adapter settings.
Right click WiFI -> Properties -> Sharing.
Under Internet Connection Sharing, enable connection sharing, disable allowing others to control sharing.

## Setup Raspberry Pi

Connect it up.
Download a GNU toolchain compatible version of Raspberry Pi OS and the toolchain https://gnutoolchains.com/raspberry64/
Download the Raspberry Pi Imager tool https://www.raspberrypi.com/software/
Burn image to SD card, boot Raspberry Pi and complete initial setup.
Choose a new password when prompted, set the keyboard layout as required, but don't connect to WiFi as we're using ethernet.

Update the OS:

```
sudo apt update && sudo apt upgrade -y && sudo apt autoremove -y
```

I'm going to change my hostname to `pi`:

```
hostnamectl set-hostname pi
hostnamectl set-hostname "pi" --pretty
```

Also edit `/etc/hosts` and change:

```
127.0.1.1 raspberrypi
```

To:

```
127.0.0.1 pi
```

Let's chack that we have `avahi-daemon`, a mDNS (multicast DNS) broadcaster installed and running so we can access the Raspberry Pi at `pi.local`.

```
sudo apt install -y avahi-daemon
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon
```

We also need to make sure we have a the `openssh-server` DNS server installed and running:

```
sudo apt install -y openssh-server
sudo systemctl enable ssh
sudo systemctl start ssh
```

Now reboot the Raspberry Pi:

```
sudo reboot now
```

## Setup SSH

### Testing the connection

On the Windows box, open PowerShell and test the SSH connection:

```
ssh pi@pi.local
```

On first login, you'll be prompted to confirm acceptance of the Raspberry Pi's fingerprint:

```
The authenticity of host 'pi.local (fe80::1a64:5dbc:8bb8:9969%11)' can't be established.
ECDSA key fingerprint is SHA256:KtWeH8KMIfnqVEntevkCmsOU2kOMnj3iaZCzaaNN3oQ.
Are you sure you want to continue connecting (yes/no/[fingerprint])?
```

You can verify the fingerprint on the Raspberry Pi, before signalling `yes`.
For me, my connection is using an ECDSA key, so I want to verify the fingerprint of the associated public key:

```
ssh-keygen -lf /etc/ssh/ssh_host_ecdsa_key.pub
```

Which outputs:

```
256 SHA256:KtWeH8KMIfnqVEntevkCmsOU2kOMnj3iaZCzaaNN3oQ root@pi (ECDSA)
```

After typing `yes` to accept the fingerprint, you'll be asked for your password.
You should now be in an interactive shell session on the Raspberry Pi.

### Passwordless login

Entering your password every time you log in via SSH is annoying so we'll setup passwordless authentication.

On the Windows box, create an SSH keypair (if you don't already have one):

```
ssh-keygen -t ecdsa -b 256 -N '""' -f $Env:USERPROFILE\.ssh\id_ecdsa
```

Linux and macOS have the `ssh-copy-id` command to copy SSH keys to a remote machine, but this isn't available on Windows.
Instead, we can run an equivalent command to copy our SSH key from Windows to the Raspberry Pi:

```
type $env:USERPROFILE\.ssh\id_ecdsa.pub | ssh pi@pi.local "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"
```

With that, you should now be able to open an SSH session to the Raspberry Pi without requiring a password:

```
ssh pi@pi.local
```

Congratulations, you've got the development Raspberry Pi setup and accessible from Windows 11 via SSH using a local domain name.
