pal_shaded = bytearray(256 * 3)

with open('bgpal.act', 'rb') as pal_file:
	for i in range(128):
		c = pal_file.read(3)
		for j in range(3):
			pal_shaded[(0   + i) * 3 + j] = c[j]
			pal_shaded[(128 + i) * 3 + j] = int(c[j] / 2)

with open('bgpal-shaded.act', 'wb') as out_file:
	out_file.write(pal_shaded)
