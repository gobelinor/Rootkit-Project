#!/bin/bash

set -e

# Vérifier si les commandes nécessaires sont installées
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

REQUIRED_COMMANDS="truncate parted losetup mkfs.ext4 mount docker grub-install qemu-system-x86_64"

for cmd in $REQUIRED_COMMANDS; do
  if ! command_exists $cmd; then
    echo "Erreur : La commande '$cmd' est requise mais n'est pas installée."
    exit 1
  fi
done

#Supprimer un potentiel disque au même nom
DISK_NAME="disk.img"

if [ -f "$DISK_NAME" ]; then
  rm -vf "$DISK_NAME"
else
  echo "No disk found."
fi

#Rentrer dans le dossier du kernel et le compiler
KERNEL_NAME="Rootkit-Project"
ROOTKIT_PATH=$(sudo find ~ -name "$KERNEL_NAME" -print -quit)

if [ -d "$ROOTKIT_PATH" ]; then
  cd "$ROOTKIT_PATH" || { echo "$ROOTKIT_PATH not found." && exit 1; }
  LINUX_NAME=$(find . -maxdepth 1 -name "linux-*" -print -quit)
  cd "$LINUX_NAME" || { echo "$LINUX_NAME not found." && exit 1; }
else
  echo "$KERNEL_NAME not found."
  exit 1
fi

make defconfig
NB_PROC=$(nproc)
USE_PROC=$((NB_PROC-1))
make -j $USE_PROC
cd ..

# Créer une image disque de 450MB
truncate -s 450M disk.img

# Créer une table de partition amorçable en mode BIOS
parted -s ./disk.img mktable msdos

# Ajouter une partition primaire ext4 utilisant tout l'espace disponible
parted -s ./disk.img mkpart primary ext4 1 "100%"

# Marquer la partition comme amorçable
parted -s ./disk.img set 1 boot on

# Configurer un périphérique loop avec détection de partition et récupérer son nom
LOOP_DEVICE=$(sudo losetup -P --show -f disk.img)
echo "Périphérique loop utilisé : $LOOP_DEVICE"

# Attendre que les partitions soient prêtes
sleep 1

# Déterminer le nom de la partition
PARTITION="${LOOP_DEVICE}p1"
if [ ! -b "$PARTITION" ]; then
  # Essayer sans 'p'
  PARTITION="${LOOP_DEVICE}1"
fi
echo "Partition utilisée : $PARTITION"

# Formater la première partition en ext4
sudo mkfs.ext4 "$PARTITION"

# Créer un répertoire de travail
mkdir -p /tmp/my-rootfs
# Monter la partition
sudo mount "$PARTITION" /tmp/my-rootfs
# Créer un script pour exécuter les commandes dans le conteneur Docker

cat << 'EOF' > ~/docker-install.sh
#!/bin/sh

# Mettre à jour les dépôts et installer les paquets nécessaires
apk update
apk add openrc
apk add util-linux
apk add build-base
apk add vim

# Configurer l'accès au terminal série via QEMU
ln -s agetty /etc/init.d/agetty.ttyS0
echo ttyS0 > /etc/securetty
rc-update add agetty.ttyS0 default
rc-update add root default

# Définir le mot de passe root (changez 'root' par le mot de passe désiré)
echo "root:root" | chpasswd

# Monter les systèmes de fichiers pseudo
rc-update add devfs boot
rc-update add procfs boot
rc-update add sysfs boot

# Copier les fichiers vers /my-rootfs
for d in bin etc lib root sbin usr; do tar c "/$d" | tar x -C /my-rootfs; done

# Créer des répertoires spéciaux
for dir in dev proc run sys var; do mkdir /my-rootfs/${dir}; done
EOF

#tricks pour modifier le docker avant grace au Dockerfile "FROM alpine COPY ./mesmodules /lib"
sudo docker build . -t my_alpine
sudo chmod +x ~/docker-install.sh

# Exécuter le conteneur Docker et le script
sudo docker run --rm -v /tmp/my-rootfs:/my-rootfs -v ~/Documents/ecole2600/docker-install.sh:/docker-install.sh my_alpine /docker-install.sh
# De retour sur le système hôte
# Copier le noyau compilé dans le répertoire boot
if [ ! -f "$LINUX_NAME"/arch/x86/boot/bzImage ]; then
  echo "Erreur : Le noyau bzImage n'a pas été trouvé à '"$LINUX_NAME"/arch/x86/boot/bzImage'."
  exit 1
fi

rm ~/Documents/ecole2600/docker-install.sh


sudo mkdir -p /tmp/my-rootfs/boot/grub
sudo cp "$LINUX_NAME"/arch/x86/boot/bzImage /tmp/my-rootfs/boot/vmlinuz

# Créer le fichier grub.cfg
cat << 'EOF' | sudo tee /tmp/my-rootfs/boot/grub/grub.cfg
serial
terminal_input serial
terminal_output serial
set root=(hd0,1)
menuentry "IMG FROM RTEAM" {
 linux /boot/vmlinuz root=/dev/sda1 console=ttyS0
}
EOF

# Installer Grub pour le démarrage BIOS
sudo grub-install --target=i386-pc --boot-directory=/tmp/my-rootfs/boot --force $LOOP_DEVICE

# Démonter la partition
sudo umount /tmp/my-rootfs

# Détacher le périphérique loop
sudo losetup -d $LOOP_DEVICE

# Exécuter QEMU sur l'image disque
share_folder="/tmp/qemu-share"
mkdir -p $share_folder

echo "Running QEMU..."
sudo qemu-system-x86_64 -hda disk.img -nographic -virtfs local,path=$share_folder,mount_tag=host0,security_model=passthrough,id=foobar 
