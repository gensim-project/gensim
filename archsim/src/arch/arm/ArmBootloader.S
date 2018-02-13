.text

b reset
b undef
b swi
b undef
b undef
b undef
b undef
b undef

reset:
	swi 0

undef:
	bkpt

swi:
	bx ip
