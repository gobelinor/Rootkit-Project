import subprocess
import argparse
import time
import select

def execute_nc_command(command):
    try:
        # Lancer le processus `nc` en mode écoute
        process = subprocess.Popen(
            ["nc", "-lvnp", "443"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        # Petite pause pour s'assurer que le port est ouvert
        time.sleep(1)
        # Envoyer la commande
        process.stdin.write(command + "\n")
        process.stdin.flush()
        # Utiliser select pour gérer le timeout et éviter le blocage
        response = []
        while True:
            # Vérifier si des données sont disponibles
            readable, _, _ = select.select([process.stdout], [], [], 0.1)  # Timeout de 5 secondes
            if not readable:
                break  # Sortir si aucune donnée disponible
            output = process.stdout.readline()
            if not output:
                break
            # print(output.strip())
            response.append(output.strip())
        # Terminer le processus
        process.terminate()
        process.wait()  # Attendre la fin du processus
        return "\n".join(response)
    except Exception as e:
        print(f"Erreur : {e}")
        return None
if __name__ == "__main__":
    # Parse l'argument de la commande
    parser = argparse.ArgumentParser()
    parser.add_argument("command", type=str, help="Command to execute")
    args = parser.parse_args()
    # Exécuter la commande spécifiée
    print("En attente de réponse ...")
    while True:
        response = execute_nc_command(args.command)
        if response:
            print("Réponse reçue :")
            print(response)
            break
        else:
            pass
