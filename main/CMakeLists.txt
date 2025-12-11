set(srcs
	"freshlist.c"
	"browser.c"
)

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS "."
	# EMBED_FILES image.jpg
)
