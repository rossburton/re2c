/* Generated by re2c */

{
	YYCTYPE yych;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *(YYMARKER = YYCURSOR);
	switch (yych) {
	case 'a':
	case 'b':
		yyt1 = yyt4 = YYCURSOR;
		goto yy3;
	case 'c':
		yyt2 = yyt4 = NULL;
		yyt1 = YYCURSOR;
		goto yy6;
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
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case 'a':
	case 'b':
		yyt4 = YYCURSOR;
		goto yy3;
	case 'c':
		yyt2 = YYCURSOR;
		goto yy6;
	default:	goto yy5;
	}
yy5:
	YYCURSOR = YYMARKER;
	goto yy2;
yy6:
	++YYCURSOR;
	yyt3 = NULL;
	{
		const size_t yynmatch = 3;
		const YYCTYPE *yypmatch[yynmatch * 2];
		yypmatch[0] = yyt1;
		yypmatch[2] = yyt4;
		yypmatch[3] = yyt2;
		yypmatch[4] = yyt3;
		yypmatch[5] = yyt3;
		yypmatch[1] = YYCURSOR;
		{}
	}
}

re2c: warning: line 6: rule matches empty string [-Wmatch-empty-string]
