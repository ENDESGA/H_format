#include "H.h"

#define H_format_version STRINGIFY( 1.4 )

group( line_type )
{
	line_unknown,
	//
	line_comment,
	line_include,
	line_define,
	line_undefine,
	line_if,
	line_else,
	line_endif,
	line_pragma
};

group( byte_type )
{
	byte_unknown,
	//
	byte_newline,
	byte_word,
	byte_value,
	byte_symbol,
	byte_open_parenthesis,
	byte_closed_parenthesis,
	byte_open_brace,
	byte_closed_brace,
	byte_comma,
	byte_period,
	byte_ellipsis,
	byte_semicolon,
	byte_backslash
};

start
{
	#if 0
		input_count = 3;
		byte ref temp_bytes_ref[] = { "", "cmake-build-debug" SEPARATOR "test.h", "output.c" };
		input_bytes_ref = temp_bytes_ref;
	#endif
	//
	if( input_count <= 1 )
	{
		print( "use: H_format [source-to-format] [optional-output]\n" );
		print( "or:  H_format version\n" );
		out executable_success;
	}
	else
	{
		if( bytes_compare( input_bytes_ref[ 1 ], "version", 7 ) is 0 )
		{
			print( "H_format version " H_format_version " (" OS_NAME ")\n" );
			out executable_success;
		}
	}
	//
	file input_file = map_file( input_bytes_ref[ 1 ], bytes_measure( input_bytes_ref[ 1 ] ) );
	//
	if_null( input_file.handle )
	{
		print( "file-mapping failed: cannot find file \"" );
		print( input_bytes_ref[ 1 ] );
		print( "\"" );
		out executable_failure;
	}
	//
	byte ref input = get_file_ref( input_file );
	byte ref input_ref = input;
	//
	declare_bytes( output, KB( 100 ) );
	temp n4 line_size = 0;
	temp flag break_word = no;
	temp flag is_assignment = no;
	temp flag is_multiline_assignment = no;
	temp line_type current_line_type = line_unknown;
	temp byte_type previous_byte_type = byte_unknown;
	//
	temp i2 parenthesis_scope = 0;
	temp i2 brace_scope = 0;
	temp i2 assignment_scope = 0;
	//
	#define output_set( BYTE ) val_of( output_ref ) = BYTE
	//
	#define output_add( BYTE )\
		START_DEF\
		{\
			output_set( BYTE );\
			++output_ref;\
			++line_size;\
		}\
		END_DEF
	//
	#define output_add_input() output_add( val_of( input_ref++ ) )
	//
	#define output_newline()\
		START_DEF\
		{\
			output_set( '\n' );\
			++output_ref;\
			line_size = 0;\
			break_word = no;\
		}\
		END_DEF
	//
	#define output_space()\
		START_DEF\
		{\
			leave_if( line_size is 0 );\
			output_add( ' ' );\
		}\
		END_DEF
	//
	#define output_indent()\
		START_DEF\
		{\
			leave_if( line_size isnt 0 );\
			repeat( brace_scope )\
			{\
				output_set( '\t' );\
				++output_ref;\
			}\
		}\
		END_DEF
	//
	// // // // // // //
	//
	check_input:
	{
		select( val_of( input_ref ) )
		{
			with( '\0' )
			{
				goto input_eof;
			}
			//
			with( ' ', '\t' )
			{
				if( line_size isnt 0 and ( previous_byte_type is byte_word or previous_byte_type is byte_value ) )
				{
					break_word = yes;
				}
				++input_ref;
				goto check_input;
			}
			//
			with( '\r' )
			{
				if( val_of( input_ref + 1 ) is '\n' )
				{
					++input_ref;
				}
			} // fall through
			with( '\n' )
			{
				if( is_assignment is yes and assignment_scope is 0 and current_line_type isnt line_define )
				{
					is_multiline_assignment = yes;
					++brace_scope;
				}
				is_assignment = no;
				//
				select( current_line_type )
				{
					with( line_define )
					{
						if( previous_byte_type is byte_backslash )
						{
							output_ref -= 2;
						}
					} // fall through
					with( line_endif )
					{
						--brace_scope;
					} // fall through
					with( line_include, line_undefine, line_if, line_else, line_pragma )
					{
						output_newline();
					}
				}
				//
				if( previous_byte_type is byte_newline or ( current_line_type is line_unknown and previous_byte_type is byte_word ) )
				{
					output_newline();
					previous_byte_type = byte_unknown;
				}
				else
				if( previous_byte_type isnt byte_unknown )
				{
					previous_byte_type = byte_newline;
				}
				//
				current_line_type = line_unknown;
				++input_ref;
				goto check_input;
			}
			//
			with( '#' )
			{
				if( line_size is 0 )
				{
					++input_ref;
					//
					preprocessor_skip_space:
					{
						select( val_of( input_ref ) )
						{
							with( ' ', '\t' )
							{
								++input_ref;
								goto preprocessor_skip_space;
							}
							//
							other leave;
						}
					}
					//
					current_line_type = line_unknown;
					//
					if( val_of( input_ref ) is 'e' )
					{
						--brace_scope;
					}
					output_indent();
					output_add( '#' );
					//
					select( val_of( input_ref ) )
					{
						with( 'i' )
						{
							if( val_of( input_ref + 1 ) is 'n' )
							{
								current_line_type = line_include;
							}
							else
							{
								current_line_type = line_if;
								++brace_scope;
							}
							leave;
						}
						//
						with( 'd' )
						{
							current_line_type = line_define;
							++brace_scope;
							leave;
						}
						//
						with( 'u' )
						{
							current_line_type = line_undefine;
							leave;
						}
						//
						with( 'p' )
						{
							current_line_type = line_pragma;
							leave;
						}
						//
						with( 'e' )
						{
							++brace_scope;
							if( val_of( input_ref + 1 ) is 'l' )
							{
								current_line_type = line_else;
							}
							else
							{
								current_line_type = line_endif;
							}
							leave;
						}
						//
						other leave;
					}
					//
					goto check_input;
				}
				//
				if( val_of( input_ref + 1 ) is '#' )
				{
					output_add_input();
					output_add_input();
					break_word = no;
					previous_byte_type = byte_symbol;
					goto check_input;
				}
				else
				{
					output_space();
					output_add_input();
					break_word = no;
					previous_byte_type = byte_symbol;
					goto check_input;
				}
			}
			//
			with( '\\' )
			{
				if( previous_byte_type is byte_backslash )
				{
					++input_ref;
					skip_backslash_gap:
					{
						select( val_of( input_ref ) )
						{
							with( ' ', '\t', '\r', '\n' )
							{
								++input_ref;
								goto skip_backslash_gap;
							}
							//
							other leave;
						}
					}
					goto check_input;
				}
				//
				if( current_line_type is line_define and parenthesis_scope is 0 )
				{
					is_assignment = no;
					if( previous_byte_type is byte_newline or previous_byte_type is byte_semicolon )
					{
						--output_ref;
					}
					previous_byte_type = byte_backslash;
					output_add_input();
					process_define_newline:
					{
						select( val_of( input_ref ) )
						{
							with( ' ', '\t', '\r', '\n' )
							{
								++input_ref;
								goto process_define_newline;
							}
							//
							other leave;
						}
					}
					output_newline();
					goto check_input;
				}
				//
				goto process_input;
			}
			//
			with( '(' )
			{
				if( ( previous_byte_type isnt byte_open_parenthesis or parenthesis_scope <= 1 ) and ( val_of( output_ref - 1 ) isnt '-' or is_assignment isnt no ) and (( parenthesis_scope isnt 0 and break_word is yes ) or previous_byte_type is byte_symbol or previous_byte_type is byte_open_parenthesis or previous_byte_type is byte_closed_parenthesis ) )
				{
					output_space();
				}
				previous_byte_type = byte_open_parenthesis;
				output_add_input();
				break_word = yes;
				++parenthesis_scope;
				goto check_input;
			}
			//
			with( ')' )
			{
				if( previous_byte_type isnt byte_open_parenthesis and ( previous_byte_type isnt byte_closed_parenthesis or parenthesis_scope <= 1 ) )
				{
					output_space();
				}
				previous_byte_type = byte_closed_parenthesis;
				output_add_input();
				break_word = yes;
				--parenthesis_scope;
				goto check_input;
			}
			//
			with( '{' )
			{
				if( parenthesis_scope isnt 0 )
				{
					goto process_input;
				}
				//
				if( is_assignment is yes )
				{
					++assignment_scope;
					goto process_input;
				}
				//
				if( val_of( input_ref + 1 ) is '}' )
				{
					output_space();
					output_add_input();
					output_add_input();
					previous_byte_type = byte_closed_brace;
					break_word = yes;
					goto check_input;
				}
				//
				if( line_size isnt 0 and current_line_type is line_define )
				{
					output_add( '\\' );
				}
				//
				if( line_size isnt 0 and previous_byte_type isnt byte_backslash )
				{
					output_newline();
				}
				output_indent();
				output_add_input();
				if( current_line_type is line_define )
				{
					output_add( '\\' );
					previous_byte_type = byte_backslash;
				}
				else
				{
					previous_byte_type = byte_open_brace;
				}
				//
				output_newline();
				++brace_scope;
				goto check_input;
			}
			//
			with( '}' )
			{
				if( parenthesis_scope isnt 0 )
				{
					goto process_input;
				}
				//
				if( is_assignment is yes and assignment_scope isnt 0 )
				{
					--assignment_scope;
					goto process_input;
				}
				//
				if( line_size isnt 0 )
				{
					if( current_line_type is line_define )
					{
						output_add( '\\' );
					}
					//
					if( previous_byte_type isnt byte_semicolon )
					{
						output_newline();
					}
				}
				--brace_scope;
				output_indent();
				output_add_input();
				if( current_line_type is line_define )
				{
					output_add( '\\' );
					previous_byte_type = byte_backslash;
				}
				else
				{
					previous_byte_type = byte_closed_brace;
				}
				//
				output_newline();
				goto check_input;
			}
			//
			with( '[' )
			{
				previous_byte_type = byte_symbol;
				output_add_input();
				break_word = yes;
				goto check_input;
			}
			//
			with( ']' )
			{
				if( val_of( output_ref - 1 ) isnt '[' )
				{
					output_space();
				}
				previous_byte_type = byte_symbol;
				output_add_input();
				break_word = yes;
				goto check_input;
			}
			//
			with( '<' )
			{
				if( current_line_type isnt line_include )
				{
					goto process_input;
				}
				//
				output_add( ' ' );
				while( val_of( input_ref ) isnt '>' )
				{
					output_add_input();
				}
				output_add_input();
				goto check_input;
			}
			//
			with( ',' )
			{
				previous_byte_type = byte_comma;
				if( parenthesis_scope is 0 and break_word is yes and current_line_type is line_define )
				{
					output_add( ' ' );
				}
				else
				if( val_of( output_ref - 1 ) is '\n' )
				{
					--output_ref;
				}
				//
				output_add_input();
				if( is_assignment is yes )
				{
					if( parenthesis_scope is 0 and assignment_scope is 0 )
					{
						output_newline();
						is_assignment = no;
					}
					else
					{
						break_word = yes;
					}
				}
				else
				if( parenthesis_scope isnt 0 or current_line_type is line_define )
				{
					break_word = yes;
				}
				else
				{
					output_newline();
				}

				goto check_input;
			}
			//
			with( '=' )
			{
				if( val_of( input_ref + 1 ) is '=' )
				{
					goto process_input;
				}
				//
				select( val_of( output_ref - 1 ) )
				{
					with( '=', '<', '>', '!', '+', '-', '*', '/', '%', '&', '|', '^' )
					{
						leave;
					}
					//
					other
					{
						output_space();
						if( parenthesis_scope is 0 )
						{
							is_assignment = yes;
						}
					}
				}
				//
				previous_byte_type = byte_symbol;
				output_add_input();
				break_word = yes;
				goto check_input;
			}
			//
			with( ':' )
			{
				if( val_of( input_ref + 1 ) is ':' )
				{
					previous_byte_type = byte_symbol;
					output_add_input();
					output_add_input();
					break_word = no;
					goto check_input;
				}
				else
				if_any( is_assignment is yes, parenthesis_scope isnt 0, previous_byte_type is byte_value, previous_byte_type is byte_symbol, previous_byte_type is byte_closed_parenthesis )
				{
					goto process_input;
				}
			} // fall through
			with( ';' )
			{
				if( is_multiline_assignment is yes )
				{
					--brace_scope;
					is_multiline_assignment = no;
				}
				//
				is_assignment = no;
				//
				if( parenthesis_scope isnt 0 )
				{
					if( previous_byte_type is byte_open_parenthesis )
					{
						output_space();
					}
					break_word = yes;
					output_add_input();
					previous_byte_type = byte_semicolon;
					goto check_input;
				}
				//
				if( previous_byte_type is byte_closed_brace )
				{
					--output_ref;
				}
				previous_byte_type = byte_semicolon;
				//
				output_add_input();
				//
				skip_semicolon_whitespace:
				{
					select( val_of( input_ref ) )
					{
						with( ' ', '\t' )
						{
							++input_ref;
							goto skip_semicolon_whitespace;
						}
						//
						other leave;
					}
				}
				//
				if( current_line_type is line_define )
				{
					if( val_of( input_ref - 1 ) is ':' )
					{
						goto check_input;
					}
					//
					if( line_size isnt 0 )
					{
						output_add( '\\' );
						previous_byte_type = byte_backslash;
					}
				}
				//
				output_newline();
				goto check_input;
			}
			//
			with( '/' )
			{
				if( val_of( input_ref + 1 ) is '/' )
				{
					is_assignment = no;
					current_line_type = line_comment;
					previous_byte_type = byte_newline;
					if( val_of( output_ref - 1 ) is '\n' )
					{
						temp byte ref scan_ref = input_ref - 1;
						check_comment_newline:
						{
							select( val_of( scan_ref ) )
							{
								with( '\n', '\r' )
								{
									output_indent();
									leave;
								}
								//
								with( ' ', '\t' )
								{
									--scan_ref;
									goto check_comment_newline;
								}
								//
								other
								{
									--output_ref;
									line_size = n4_max;
									leave;
								}
							}
						}
					}
					else
					{
						output_indent();
					}
					//
					output_space();
					output_add_input();
					output_add_input();
					//
					process_comment:
					{
						select( val_of( input_ref ) )
						{
							with( '\0' )
							{
								goto input_eof;
							}
							//
							with( '\r' )
							{
								if( val_of( input_ref + 1 ) is '\n' )
								{
									++input_ref;
								}
							} // fall through
							with( '\n' )
							{
								output_newline();
								++input_ref;
								goto check_input;
							}
							//
							other
							{
								output_add_input();
								goto process_comment;
							}
						}
					}
				}
				else
				if( val_of( input_ref + 1 ) is '*' )
				{
					output_indent();
					output_space();
					output_add_input();
					output_add_input();
					//current_line_type = line_comment;
					process_multi_comment:
					{
						select( val_of( input_ref ) )
						{
							with( '\0' )
							{
								goto input_eof;
							}
							//
							with( '*' )
							{
								if( val_of( input_ref + 1 ) is '/' )
								{
									output_add_input();
									output_add_input();

									process_multi_comment_end:
									{
										select( val_of( input_ref ) )
										{
											with( '\0' )
											{
												goto input_eof;
											}
											//
											with( '\r' )
											{
												if( val_of( input_ref + 1 ) is '\n' )
												{
													++input_ref;
												}
											} // fall through
											with( '\n' )
											{
												output_newline();
												++input_ref;
												goto check_input;
											}
											//
											with( ' ', '\t' )
											{
												++input_ref;
												goto process_multi_comment_end;
											}
											//
											other
											{
												goto check_input;
											}
										}
									}

									goto check_input;
								}
								// else fall through
							}
							//
							other
							{
								output_add_input();
								goto process_multi_comment;
							}
						}
					}
				}
				else
				{
					goto process_input;
				}
			}
			//
			with( '\'' )
			{
				previous_byte_type = byte_symbol;
				output_indent();
				if( break_word is yes )
				{
					output_space();
					break_word = no;
				}
				output_add_input();
				//
				if( val_of( input_ref ) is '\\' )
				{
					output_add_input();
				}
				output_add_input();
				output_add_input();
				break_word = yes;
				goto check_input;
			}
			//
			with( '"' )
			{
				previous_byte_type = byte_symbol;
				output_indent();
				output_space();
				output_add_input();
				//
				process_bytes:
				{
					select( val_of( input_ref ) )
					{
						with( '"' )
						{
							output_add_input();
							leave;
						}
						//
						with( '\\' )
						{
							output_add_input();
						}
						//
						other
						{
							output_add_input();
							goto process_bytes;
						}
					}
				}
				break_word = yes;
				goto check_input;
			}
			//
			with( '+' )
			{
				if( val_of( input_ref + 1 ) is '+' )
				{
					output_indent();
					if( previous_byte_type isnt byte_word )
					{
						output_space();
					}
					output_add_input();
					output_add_input();
					break_word = no;
					goto check_input;
				}
				//
				goto process_input;
			}
			//
			with( '-' )
			{
				select( val_of( input_ref + 1 ) )
				{
					with( '-' )
					{
						output_indent();
						if( previous_byte_type isnt byte_word )
						{
							output_space();
						}
					} // fall through
					with( '>' )
					{
						output_add_input();
						output_add_input();
						break_word = no;
						goto check_input;
					}
					//
					other
					{
						goto process_input;
					}
				}
			}
			//
			with( '.' )
			{
				break_word = no;
				if( val_of( input_ref + 1 ) is '.' )
				{
					previous_byte_type = byte_ellipsis;

					if( previous_byte_type isnt byte_word )
					{
						output_space();
					}

					output_add_input();
					output_add_input();
					output_add_input();
					goto check_input;
				}
				else if( previous_byte_type is byte_word or previous_byte_type is byte_symbol )
				{
					output_add_input();
					goto check_input;
				}
				//
				previous_byte_type = byte_period;
				output_space();
				output_add_input();
				goto check_input;
			}
			//
			other
			{
				process_input:
				{
					output_indent();
					//
					temp byte in_byte = val_of( input_ref );
					if( byte_is_letter( in_byte ) or in_byte is '_' )
					{
						if( val_of( input_ref - 1 ) is '*' or val_of( input_ref - 1 ) is '&' or val_of( input_ref - 1 ) is '!' )
						{
							break_word = no;
						}
						//
						if( break_word is yes )
						{
							output_space();
							break_word = no;
						}
						previous_byte_type = byte_word;
						do
						{
							output_add_input();
							in_byte = val_of( input_ref );
						}
						while( byte_is_letter( in_byte ) or byte_is_number( in_byte ) or in_byte is '_' );
					}
					else
					if( byte_is_number( in_byte ) )
					{
						if( val_of( input_ref - 1 ) is '-' )
						{
							break_word = no;
						}
						//
						if( break_word is yes )
						{
							output_space();
							break_word = no;
						}
						previous_byte_type = byte_value;
						do
						{
							output_add_input();
							in_byte = val_of( input_ref );
						}
						while( byte_is_letter( in_byte ) or byte_is_number( in_byte ) or in_byte is '.' );
					}
					else
					{
						// symbol
						if( val_of( input_ref - 1 ) isnt val_of( input_ref ) )
						{
							output_space();
						}
						//
						break_word = yes;
						//
						previous_byte_type = byte_symbol;
						output_add_input();
					}
					//
					goto check_input;
				}
			}
		}
	}
	//
	input_eof:
	if( val_of( output_ref - 1 ) isnt '\n' )
	{
		output_newline();
	}
	byte temp_input_path[ PATH_MAX_SIZE ] = "";
	temp n2 temp_input_path_size = input_file.path_size;
	bytes_copy( input_file.path, temp_input_path_size, temp_input_path );
	file_unmap( ref_of( input_file ) );
	//
	print( "formatting: " );
	print( temp_input_path );
	//
	file output_file;
	if( input_count is 3 )
	{
		output_file = open_file_write( input_bytes_ref[ 2 ], bytes_measure( input_bytes_ref[ 2 ] ) );
		//
		print( "\noutput: " );
		print( output_file.path );
	}
	else
	{
		output_file = open_file_write( temp_input_path, temp_input_path_size );
	}
	//
	file_save( ref_of( output_file ), output, output_ref - output );
	file_close( ref_of( output_file ) );
	print_nl();
	out executable_success;
}
