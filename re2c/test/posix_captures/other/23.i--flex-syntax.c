/* Generated by re2c */

{
	YYCTYPE yych;
	if ((YYLIMIT - YYCURSOR) < 3) YYFILL(3);
	yych = *YYCURSOR;
	switch (yych) {
	case 'y':
		yyt1 = yyt2 = YYCURSOR;
		goto yy3;
	default:
		yyt1 = yyt2 = yyt3 = YYCURSOR;
		goto yy2;
	}
yy2:
	{
		const size_t yynmatch = 2;
		const YYCTYPE *yypmatch[yynmatch * 2];
		yypmatch[0] = yyt1;
		yypmatch[2] = yyt2;
		yypmatch[3] = yyt3;
		yypmatch[1] = YYCURSOR;
		{}
	}
yy3:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'y':
		yyt2 = YYCURSOR;
		goto yy4;
	default:
		yyt3 = YYCURSOR;
		goto yy2;
	}
yy4:
	yych = *++YYCURSOR;
	yyt3 = YYCURSOR;
	switch (yych) {
	case 'y':	goto yy5;
	default:	goto yy2;
	}
yy5:
	++YYCURSOR;
	goto yy2;
}

re2c: warning: line 5: rule matches empty string [-Wmatch-empty-string]
re2c: warning: line 6: rule matches empty string [-Wmatch-empty-string]
re2c: warning: line 6: unreachable rule (shadowed by rule at line 5) [-Wunreachable-rules]
