#!/bin/bash

set -e

# Vérifier si les commandes nécessaires sont installées
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

REQUIRED_COMMANDS="qemu-img truncate parted losetup mkfs.ext4 mount docker grub-install qemu-system-x86_64"
for cmd in $REQUIRED_COMMANDS; do
  if ! command_exists $cmd; then
    echo "Erreur : La commande '$cmd' est requise mais n'est pas installée."
    exit 1
  fi
done

# Nom des fichiers
QCOW2_IMAGE="disk.qcow2"
IMG_IMAGE="disk.img"
SHARE_FOLDER="/tmp/qemu-share"

# Vérifier l'existence de l'image qcow2
if [ ! -f "$QCOW2_IMAGE" ]; then
  echo "Erreur : L'image '$QCOW2_IMAGE' n'existe pas. Veuillez la créer avant de lancer ce script."
  exit 1
fi

# Conversion de .qcow2 en .img
echo "Conversion de $QCOW2_IMAGE en $IMG_IMAGE..."
qemu-img convert -f qcow2 -O raw "$QCOW2_IMAGE" "$IMG_IMAGE"

# Configurer un périphérique loop
LOOP_DEVICE=$(sudo losetup -P --show -f "$IMG_IMAGE")
echo "Périphérique loop utilisé : $LOOP_DEVICE"

# Attendre que les partitions soient prêtes
sleep 1

# Déterminer le nom de la partition
PARTITION="${LOOP_DEVICE}p1"
if [ ! -b "$PARTITION" ]; then
  PARTITION="${LOOP_DEVICE}1"
fi
echo "Partition utilisée : $PARTITION"

# Monter la partition pour vérification
echo "Montage de la partition pour vérification..."
mkdir -p /tmp/my-rootfs
sudo mount "$PARTITION" /tmp/my-rootfs

# Vérifier si le système de fichiers et le noyau sont en place
if [ ! -f /tmp/my-rootfs/boot/vmlinuz ] || [ ! -f /tmp/my-rootfs/boot/grub/grub.cfg ]; then
  echo "Erreur : Les fichiers nécessaires (vmlinuz, grub.cfg) sont absents de la partition."
  sudo umount /tmp/my-rootfs
  sudo losetup -d "$LOOP_DEVICE"
  exit 1
fi

# Démonter la partition
sudo umount /tmp/my-rootfs

# Détacher le périphérique loop
sudo losetup -d "$LOOP_DEVICE"

# Créer un dossier partagé pour QEMU
mkdir -p "$SHARE_FOLDER"

# Lancer QEMU avec l'image convertie
echo "Lancement de QEMU avec $IMG_IMAGE..."
sudo qemu-system-x86_64 \
  -drive file="$IMG_IMAGE",format=raw \
  -nographic \
  -virtfs local,path="$SHARE_FOLDER",mount_tag=host0,security_model=passthrough,id=foobar \
  # -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
  # -serial mon:stdio

