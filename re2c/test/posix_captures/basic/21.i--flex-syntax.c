/* Generated by re2c */

{
	YYCTYPE yych;
	if ((YYLIMIT - YYCURSOR) < 3) YYFILL(3);
	yych = *(YYMARKER = YYCURSOR);
	switch (yych) {
	case 'a':	goto yy3;
	default:	goto yy2;
	}
yy2:
	{
		const size_t yynmatch = 1;
		const YYCTYPE *yypmatch[yynmatch * 2];
		yypmatch[0] = YYCURSOR;
		yypmatch[1] = YYCURSOR;
		{}
	}
yy3:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'b':	goto yy5;
	default:	goto yy4;
	}
yy4:
	YYCURSOR = YYMARKER;
	goto yy2;
yy5:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'c':	goto yy6;
	default:	goto yy4;
	}
yy6:
	++YYCURSOR;
	{
		const size_t yynmatch = 4;
		const YYCTYPE *yypmatch[yynmatch * 2];
		yypmatch[0] = YYCURSOR - 3;
		yypmatch[1] = YYCURSOR;
		yypmatch[2] = YYCURSOR - 3;
		yypmatch[3] = YYCURSOR - 2;
		yypmatch[4] = YYCURSOR - 2;
		yypmatch[5] = YYCURSOR - 1;
		yypmatch[6] = YYCURSOR - 1;
		yypmatch[7] = YYCURSOR;
		{}
	}
}

re2c: warning: line 6: rule matches empty string [-Wmatch-empty-string]
