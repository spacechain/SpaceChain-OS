#ifndef BISON_NSPARSER_H
# define BISON_NSPARSER_H

#ifndef YYSTYPE
typedef union {
	char *str;
	int   mapval;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NL	257
# define	SUCCESS	258
# define	UNAVAIL	259
# define	NOTFOUND	260
# define	TRYAGAIN	261
# define	RETURN	262
# define	CONTINUE	263
# define	ERRORTOKEN	264
# define	STRING	265


extern YYSTYPE _nsyylval;

#endif /* not BISON_NSPARSER_H */