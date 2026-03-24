# Carte de puissance pour le robot

## Cahier des charges
### Général
 - Connecter une batterie jusqu'à 6s avec un connecteur XT60
 - Génerer une tension 5v
 - Avoir un connecteur pour brancher un convertisseur de tension externe (qu'on appelera V')
 - Alimenter 3 autres cartes, en 3 tensions différentes (Vbatt, 5v, et V')
 - Avoir une sortie dédié à l'alimentation du Pi (sortie en Vbatt)
 - Avoir un bouton (ou entrée bouton) pour l'allumage du système (permet au système de s'éteindre complétement en cas de batterie faible)

### Sécurité du robot
 - Brancher le Bouton d'Arrêt d'Urgence (BAU)
 - Être capable de couper l'alimentation Vbatt et 5v allant vers les cartes annexes
 - Signaler l'état du BAU au Pi (à ROS)

### Protection de la batterie
 - Être capable de couper l'alimentation du Pi
 - Connaitre l'état d'alimentation du Pi (est-il allumé ?)
 - Être capable de couper l'alimentation générale
 - Diode de protection
 - Fusible

### Affichage/feedback
 - Afficher l'état de charge de la batterie (pourcentage sur afficheur 7 segment)
 - Avertissement sonore lorsque la batterie est à plat
 - LED RGB indiquant l'état du système intégré au bouton de mise en marche

### Possibilitées
 - Mesurer le courant sortant de la batterie
