	if ((filter & ALT_FILTER)  > 0) {

		#if ORDER1
		// initialize the scaled parameters by shifting left, by b0 = (1*2^14)
		static int16_t a0_l = 4018;   //0.2452
		static int16_t a1_l = 4018;   //0.2452
		//static int16_t b0_l = 16384;  //1
		static int16_t b1_l = -8348;  //-0.5095
		// todo: will thislbe okay, everytime function is called?
		static int16_t x0_l = 0; 
		static int16_t x1_l = 0; 
		static int16_t y0_l = 0;
		static int16_t y1_l = 0;

		x0_l  = saz; 										// take current raw sample
		y0_l  = (MUL_SCALED(a0_l, x0_l, 14) + MUL_SCALED(a1_l, x1_l, 14)
		       - MUL_SCALED(b1_l, y1_l, 14));		 		//implement the filter
		saz_f = y0_l;										// extract filtered value
		x1_l  = x0_l;
		y1_l  = y0_l; 
		
		#else

		static int16_t a0_l =  1105;    
		static int16_t a1_l =  2210;    
		static int16_t a2_l =  1105;   
		//static int16_t b0_l =  16384;  
		static int16_t b1_l =  -18727;
		static int16_t b2_l =  6763;

		static int16_t x0_l = 0; 
		static int16_t x1_l = 0;
		static int16_t x2_l = 0;

		static int16_t y0_l = 0;
		static int16_t y1_l = 0;
		static int16_t y2_l = 0;

		x0_l = saz;

		y0_l = (  MUL_SCALED(a0_l, x0_l, 14) 
				+ MUL_SCALED(a1_l, x1_l, 14)
				+ MUL_SCALED(a2_l, x2_l, 14) 
				- MUL_SCALED(b1_l, y1_l, 14) 
				- MUL_SCALED(b2_l, y2_l, 14) 
		       );
		saz_f = y0_l;

		x2_l = x1_l;
		y2_l = y1_l;

		x1_l = x0_l;
		y1_l = y0_l;

		#endif
	}