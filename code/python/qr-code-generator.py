import qrcode

# Wlan
wifi_qr = "WIFI:T:WPA;S:ReactionGame;P:reaction-game;;"

img = qrcode.make(wifi_qr)
img.save("wifi_qr.png")

# URL
url = "http://reaction-game.local"

img = qrcode.make(url)
img.save("reaction_game_qr.png")
