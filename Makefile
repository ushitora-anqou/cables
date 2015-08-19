ifeq ($(OS),Windows_NT)
	#Windows 上で実行された
 	include Makefile.win
else
	include Makefile.lin
endif
