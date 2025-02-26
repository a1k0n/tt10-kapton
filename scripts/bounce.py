logo_x = 128
logo_y = 128

screen_x = 640
screen_y = 480

frames = 640

x, y = 0, 0
dx, dy = 2, 2

for i in range(frames):
    # print(i, x, y, dx, dy)
    if x + dx > (screen_x - logo_x) or x + dx < 0:
        dx = -dx
    if y + dy > (screen_y - logo_y) or y + dy < 0:
        dy = -dy
    x += dx
    y += dy

print(x, y, -dx, -dy)
