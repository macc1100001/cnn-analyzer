#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <string>
#include "ImGuiFileDialog.h"

#include "network.h"
#include "list.h"
#include "option_list.h"
#include "parser.h"



// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

bool generate_featureMaps(network *net, char *outfile, char *filename, GLuint* out_texture, int* out_width, int* out_height, int layer_i, int filter){
	
	int image_width = 0;
    int image_height = 0;	
    int image_channels = 0;
	unsigned char* image_data1 = stbi_load(filename, &image_width, &image_height, &image_channels, 3);
    if (image_data1 == NULL)
        return false;
	/*
		TODO:			
		Convert imageData to "unsigned char*":
		
		unsigned char* data = (unsigned char*)xcalloc(im.w * im.h * im.c, sizeof(unsigned char));
		int i, k;
		for (k = 0; k < im.c; ++k) {
        	for (i = 0; i < im.w*im.h; ++i) {
        	    data[i*im.c + k] = (unsigned char)(255 * im.data[i + k*im.w*im.h]);
        	}
    	}
    	
    	To show image maybe use this?:
	    	void show_image(image p, const char *name)
    	To save the image use this:
    		void save_image_png(image im, const char *name)
    		
    	To get output of a layer use this
    		For GPU:
    		float * a = get_network_output_layer_gpu(net, i);
			For CPU:
			float * a = net.layers[i].output;
	*/
	// Example with CPU
	
	int i, j, k;
	image im = make_image(image_width, image_height, image_channels);
    for(k = 0; k < image_channels; ++k){
        for(j = 0; j < image_height; ++j){
            for(i = 0; i < image_width; ++i){
                int dst_index = i + image_width*j + image_width*image_height*k;
                int src_index = k + image_channels*i + image_channels*image_width*j;
                im.data[dst_index] = (float)image_data1[src_index]/255.;
            }
        }
    }
    
    // resize
    image sized = resize_image(im, net->w, net->h);
	
	
	network_state state = {0};
    state.net = *net;
    state.index = 0;
    state.input = sized.data;
    state.truth = 0;
    state.train = 0;
    state.delta = 0;
	
	state.workspace = net->workspace;
	//forward_custom
    for(i = 0; i < net->n; ++i){
        state.index = i;
        layer l = net->layers[i];
        l.forward(l, state);
        state.input = l.output;
    }
	
	
	int h, w;
	h = net->layers[layer_i].out_h;
	w = net->layers[layer_i].out_w;
	//c = net->layers[i-1].out_c;
	
	//unsigned char* image_data = (unsigned char*)xcalloc(w * h * c, sizeof(unsigned char));
	
	float *out = net->layers[layer_i].output;
	
	/*for (k = 0; k < c; ++k) {
    	for (i = 0; i < w*h; ++i) {
    	    image_data[k*h*w + i] = (unsigned char)(255 * out[i + k*w*h]);
    	}
	}
	*/
	
	 // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_FLOAT, out+(filter*h*w));
    stbi_image_free(image_data1);

    *out_texture = image_texture;
    *out_width = w;
    *out_height = h;
			
	free_image(im);
	//free_network(net);
	return true;
}


int main(int, char**){
	// Setup SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}
    // Decide GL+GLSL versions
	#if defined(IMGUI_IMPL_OPENGL_ES2)
		// GL ES 2.0 + GLSL 100
		const char* glsl_version = "#version 100";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#elif defined(__APPLE__)
		// GL 3.2 Core + GLSL 150
		const char* glsl_version = "#version 150";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	#else
		// GL 3.0 + GLSL 130
		const char* glsl_version = "#version 130";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#endif
    // From 2.0.18: Enable native IME.
	#ifdef SDL_HINT_IME_SHOW_UI
		SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	#endif
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
	SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* window = SDL_CreateWindow("CNN Analyzer", SDL_WINDOWPOS_CENTERED,
	SDL_WINDOWPOS_CENTERED, 1600, 900, windowFlags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // VSync
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);
	
	
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	bool done = false;
	bool show_demo_window = false;
	network net;
	
	while(!done){
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			ImGui_ImplSDL2_ProcessEvent(&event);
			if(event.type == SDL_QUIT)
				done = true;
			if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID ==
			SDL_GetWindowID(window))
				done = true;
		}
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		
		if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
	
			
			ImGuiIO *ioptr = &io;
			ImVec2 maxSize;
			maxSize.x = ioptr->DisplaySize.x * 0.85f;
			maxSize.y = ioptr->DisplaySize.y * 0.85f;
			ImVec2 minSize;
			minSize.x = maxSize.x * 0.65f;
			minSize.y = maxSize.y * 0.65f;
			ImGuiFileDialogFlags dialogFlags =  ImGuiFileDialogFlags_DisableCreateDirectoryButton | ImGuiFileDialogFlags_DontShowHiddenFiles | ImGuiFileDialogFlags_Modal;
			static std::string darknetPath = "", objdataPath = "", cfgFilePath = "", weightsFilePath = "";
			ImGui::Begin("Network data", NULL, ImGuiWindowFlags_AlwaysAutoResize);
			
			
			ImGui::Text("Darknet directory");
			ImGui::SameLine();
			static int clicked = 0;
			ImGui::PushID(1);
			if(ImGui::Button("Open..."))
				clicked++;
			
			if(clicked & 1){
				// Make file dialog popup to the darkent route
				ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose Directory", nullptr, ".",
														0,0,1,nullptr, dialogFlags);
				if(ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey", ImGuiWindowFlags_NoCollapse,
														minSize, maxSize)){
					if(ImGuiFileDialog::Instance()->IsOk()){
						darknetPath = ImGuiFileDialog::Instance()->GetFilePathName();
						//filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
					}
					clicked++;
					ImGuiFileDialog::Instance()->Close();
				}
			}
			if(!darknetPath.empty())
					ImGui::Text("%s", darknetPath.c_str());

			ImGui::PopID();
			
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("obj.data file");
			ImGui::SameLine();
			
			static int clicked1 = 0;
			ImGui::PushID(2);
			if(ImGui::Button("Open..."))
				clicked1++;		
					
			static int item_current = 0;
			const char* items[] = {"train", "valid", "test"};
			static list *options = NULL;
			
			static bool openFileContents = false;
			static bool openImage = false;
			static int my_image_width = 0;
			static int my_image_height = 0;
			static int fm_texture_w = 0;
			static int fm_texture_h = 0;
			static GLuint my_image_texture;
			static GLuint fm_texture;
			static char imageName[256];
			static char **imagePaths = NULL;
			static bool showComboBox = false;
			
			static bool showFeatureMaps = false;
			
			static char fileContents[4096];
			static char temp[2048];
			
			if(clicked1 & 1){
				static char defaultDataFile[512] = {"\0"};
				static char dataPath[] = "/data/";
				if(strlen(defaultDataFile) == 0 && !darknetPath.empty()){
					strncpy(defaultDataFile, darknetPath.c_str(), darknetPath.size());
					strncat(defaultDataFile, dataPath, strlen(dataPath));
				}
				else if(darknetPath.empty())
					strncpy(defaultDataFile, ".", 2);
				// Again make file dialog popup then store the necesary info from obj.data
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose ojb.data File", 
														".data", defaultDataFile,0,0,1,nullptr, dialogFlags);
				if(ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, 
														minSize, maxSize)){
					if(ImGuiFileDialog::Instance()->IsOk()){
						objdataPath = ImGuiFileDialog::Instance()->GetFilePathName();
						//filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
					}
					clicked1++;
					ImGuiFileDialog::Instance()->Close();
				}
				
				if(!objdataPath.empty()){
					memset(fileContents, 0, 4096);
					FILE *fp = fopen(objdataPath.c_str(), "r");
						if(fp){
							fread(fileContents, IM_ARRAYSIZE(fileContents), sizeof(*fileContents), fp);
							fclose(fp);
							openFileContents = true;
						}
					memset(temp, 0, 2048);
					strncpy(temp, objdataPath.c_str(), objdataPath.size());	
					showComboBox = true;		
				}// if(!objdataPath.empty)
			}
			
			if(showComboBox)
				ImGui::Combo("Datasets", &item_current, items, IM_ARRAYSIZE(items));
			
			static int current_comboSel = -1;
			char* val;
			static char imagesPaths[1000*512];
			static list *finalImagesPaths = NULL;
			static char full_valPath[512];
			static char **layerNamesArr = NULL;
			static int layerNamesArr_size = 0;
			static int layer_current = 0;
			static int filter_current = 0;
			static char imgPath[512];
			
			if(current_comboSel != item_current && strlen(temp) != 0){
				options = read_data_cfg(temp);
				current_comboSel = item_current;
				char item[1024] = {"\0"};
				strncpy(item, items[item_current], strlen(items[item_current]));
				val = option_find(options, item);
				free_list(options);
			
				memset(full_valPath, 0, 512);
				strncpy(full_valPath, darknetPath.c_str(), darknetPath.size());
				strncat(full_valPath, "/", 2);
				strncat(full_valPath, val, strlen(val));
				memset(imagesPaths, 0, 1000*512);
				FILE *fp = fopen(full_valPath, "r");
				if(fp){
					fread(imagesPaths, IM_ARRAYSIZE(imagesPaths), sizeof(*imagesPaths), fp);
					fclose(fp);
				}
									
				if(imagePaths)
					free(imagePaths);
				
									
				if(finalImagesPaths)
					free_list(finalImagesPaths);
				

				finalImagesPaths = make_list();
					char *str1 = imagesPaths, *saveptr, *token;
					for(int j = 1; ;j++){
						token = strtok_r(str1, "\n", &saveptr);
						if(token == NULL)
							break;
						list_insert(finalImagesPaths, token);	
						str1 = NULL;
					}
				
				imagePaths = (char**)list_to_array(finalImagesPaths);
				
			}// if(current_comboSel != item_current)
			
			if(finalImagesPaths != NULL && imagePaths != NULL){
				static int current_item_idx = 0;	
				
				if(ImGui::BeginListBox("Images", ImVec2(ImGui::CalcItemWidth() * 1.1, 
												5 * ImGui::GetTextLineHeightWithSpacing()))){
					for(int n = 0; n < finalImagesPaths->size; n++){
						const bool is_selected = (current_item_idx == n);
						
						char nameList[256];
						char *nameIdx = rindex(imagePaths[n], '/');
						int pos = (nameIdx - imagePaths[n]);
						memset(nameList, 0, 256);
						size_t nameLen = strlen(imagePaths[n]) - (pos+1);
						strncpy(nameList, nameIdx+1, nameLen);
						
											
						if(ImGui::Selectable(nameList, is_selected)){
							current_item_idx = n;
							
							//char imgPath[512];
							memset(imgPath, 0, 512);
							strncpy(imgPath, darknetPath.c_str(), darknetPath.size());
							strncat(imgPath, "/", 2);
							strncat(imgPath, imagePaths[current_item_idx],
											 strlen(imagePaths[current_item_idx]));
							my_image_width = 0;
							my_image_height = 0;
							
							fm_texture_h = 0;
							fm_texture_w = 0;
							
					        glDeleteTextures(1, &my_image_texture);
					        glDeleteTextures(1, &fm_texture);
							
							my_image_texture = 0;
							fm_texture = 0;
							
							bool ret = LoadTextureFromFile(imgPath,	&my_image_texture,
															&my_image_width, &my_image_height);
							IM_ASSERT(ret);
							
														
							memset(imageName, 0, 256);
							strncpy(imageName, nameList, strlen(nameList));
							
							// mostrar imagen desde RAM
							if(!cfgFilePath.empty() && !weightsFilePath.empty()){
								bool ret2 = generate_featureMaps(&net, NULL, imgPath, 
											&fm_texture, &fm_texture_w, &fm_texture_h, 1, 0);
							
								IM_ASSERT(ret2);
							}
							
							openImage = true;
							showFeatureMaps = true;
						}
						if(is_selected){
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndListBox();
					
				} //EndListBox
			}
			
			
			ImGui::PopID();
			
			static int clicked2 = 0;
			static bool openFileContents1 = false;
			static char fileContents1[4096*100];
			static char cfgtmp[] = {0};
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("cfg file");	
			ImGui::SameLine();
			ImGui::PushID(3);
			if(ImGui::Button("Open..."))
				clicked2++;
				
			if(clicked2 & 1){
				static char defaultCfgFile[1024] = {"\0"};
				if(strlen(defaultCfgFile) == 0 && !darknetPath.empty()){
					strncpy(defaultCfgFile, darknetPath.c_str(), darknetPath.size());
				}
				else if(darknetPath.empty())
					strncpy(defaultCfgFile,".", 2);
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose .cfg file",
														".cfg", defaultCfgFile, 0, 0, 1, nullptr, dialogFlags);
				if(ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse,
														minSize, maxSize)){
					if(ImGuiFileDialog::Instance()->IsOk()){
						cfgFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
						if(!cfgFilePath.empty()){
							memset(fileContents1, 0, 4096*100);
							FILE *fp = fopen(cfgFilePath.c_str(), "r");
							if(fp){
								fread(fileContents1, IM_ARRAYSIZE(fileContents1), sizeof(*fileContents1), fp);
								fclose(fp);
								openFileContents1 = true;
							}
							memset(cfgtmp, 0, 256);
							char cfg[256] = {0};
							strncpy(cfg, cfgFilePath.c_str(), 256);
							char *idx = rindex(cfg, '/');
							size_t len = cfgFilePath.size() - (idx-cfg);
							strncpy(cfgtmp, idx+1, len);	
							
							net = parse_network_cfg_custom(cfg, 1, 1);
							
							char validx[10] = {0};
							list *layerNames = make_list();
							for(int i = 0; i < net.n; ++i){
								layer l = net.layers[i];
								if(l.type == CONVOLUTIONAL){
									char *nameLayer = (char*)xcalloc(30, sizeof(char));
									sprintf(validx,"%d", i);
									strncat(nameLayer, "Conv-", 6);
									strncat(nameLayer, validx, strlen(validx));
									list_insert(layerNames, nameLayer);
								}
								else if(l.type == SHORTCUT){
									char *nameLayer = (char*)xcalloc(30, sizeof(char));;
									sprintf(validx,"%d", i);
									strncat(nameLayer, "Shortcut-", 10);
									strncat(nameLayer, validx, strlen(validx));
									list_insert(layerNames, nameLayer);
								}						
							}
							layerNamesArr_size = layerNames->size;					
							layerNamesArr = (char**)list_to_array(layerNames);
							free(layerNames);
						}		
					}
					clicked2++;				
					ImGuiFileDialog::Instance()->Close();
				}
			}
			if(!cfgFilePath.empty())
				ImGui::Text("%s", cfgtmp);
			
			ImGui::PopID();
			
			static int clicked3 = 0;
			static char wtmp[256] = {0};
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("weights file");
			ImGui::SameLine();
			ImGui::PushID(4);
			if(ImGui::Button("Open..."))
				clicked3++;
				
			if(clicked3 & 1){
				char defaultWeigthsDir[512] = {"\0"};
				// replace this later with directory in "backup = " in obj.data
				char replaceThisLater[] = "/backup/";
				if(!darknetPath.empty()){
					strncpy(defaultWeigthsDir, darknetPath.c_str(), darknetPath.size());
					strncat(defaultWeigthsDir, replaceThisLater, strlen(replaceThisLater));
				}
				else
					strncpy(defaultWeigthsDir, ".", 2);
					
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose weights file",
														".weights", defaultWeigthsDir, 0, 0, 1, nullptr, dialogFlags);
				if(ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse,
														minSize, maxSize)){
					if(ImGuiFileDialog::Instance()->IsOk()){
						weightsFilePath = ImGuiFileDialog::Instance()->GetFilePathName();		
						if(!weightsFilePath.empty()){
							memset(wtmp, 0, 256);
							char wfp[256] = {0};
							strncpy(wfp, weightsFilePath.c_str(), 256);
							char *idx = rindex(wfp, '/');
							size_t len = weightsFilePath.size() - (idx-wfp);
							strncpy(wtmp, idx+1, len);
							
							load_weights(&net, wfp);
						}				
					}
					clicked3++;
					ImGuiFileDialog::Instance()->Close();
				}
				/*if(!weightsFilePath.empty()){
					memset(wtmp, 0, 256);
					char wfp[256] = {0};
					strncpy(wfp, weightsFilePath.c_str(), 256);
					char *idx = rindex(wfp, '/');
					size_t len = weightsFilePath.size() - (idx-wfp);
					strncpy(wtmp, idx+1, len);
					
					load_weights(&net, wfp);
				}*/
			}
			if(!weightsFilePath.empty()){
				ImGui::Text("%s", wtmp);
			}
			
			ImGui::PopID();
									
			ImGui::End();
			
			static int current_comboSel_layer = -1;
			static int current_comboSel_filter = -1;
			
			if(openFileContents){
				ImGui::SetNextWindowSize(ImVec2(425, 130), ImGuiCond_Always);
				ImGui::Begin("Contents of \"obj.data\"", &openFileContents);		
				ImGui::TextWrapped("%s",fileContents);
				ImGui::End();
			}
			
			if(openFileContents1){
				ImGui::SetNextWindowSize(ImVec2(225, 630), ImGuiCond_Appearing);
				ImGui::Begin("Contents of cfg file", &openFileContents1);		
				ImGui::TextWrapped("%s",fileContents1);
				ImGui::End();
			}
			
			if(openImage){
				ImGui::Begin("Image", &openImage);
				ImGui::Text("%s", imageName);
				//ImGui::Text("pointer = %p", my_image_texture);
				ImGui::Text("size = %0.0f x %0.0f", my_image_width*0.5f, my_image_height*0.5f);
				ImGui::Image((void*)(intptr_t)my_image_texture, 
							ImVec2(my_image_width*0.5f, my_image_height*0.5f),
							ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
				ImGui::End();		
			}
			//static char filterNames[5000*10] = {0};
			static int filterNames_size = 0;
			static char **filterNamesArr = NULL;
			if(showFeatureMaps){
				ImGui::Begin("Feature maps", &showFeatureMaps);
				// get layer index from layer nameFilter
				char *tmp = rindex(layerNamesArr[layer_current], '-');
				char layer_toShow[4];
				strncpy(layer_toShow, tmp+1, strlen(tmp+1));
				int layer_toShowInt = atoi(layer_toShow);
				layer l = net.layers[layer_toShowInt];
				
				list *filterNames = make_list();
				for(int i = 0; i < l.out_c; ++i){
					char *nameFilter = (char*)xcalloc(10, sizeof(char));
					sprintf(nameFilter, "%d", i);
					list_insert(filterNames, nameFilter);
				}
				filterNames_size = filterNames->size;
				filterNamesArr = (char**)list_to_array(filterNames);
				free(filterNames);
				
				ImGui::Text("Size = %d x %d x %d", l.out_w, l.out_h, l.out_c);
				ImGui::Combo("Layers", &layer_current, layerNamesArr, layerNamesArr_size);
				ImGui::Combo("Filters", &filter_current, filterNamesArr, filterNames_size);
				if(current_comboSel_layer != layer_current || current_comboSel_filter != filter_current){
					current_comboSel_layer = layer_current;
					current_comboSel_filter = filter_current;
					fm_texture_h = 0;
					fm_texture_w = 0;
					glDeleteTextures(1, &fm_texture);
					fm_texture = 0;
					if(!cfgFilePath.empty() && !weightsFilePath.empty()){
						bool ret2 = generate_featureMaps(&net, NULL, imgPath, 
										&fm_texture, &fm_texture_w, &fm_texture_h, layer_current, filter_current);
							
						IM_ASSERT(ret2);
					}
					
				}
				float factor;
				factor = 512.0/fm_texture_h;
				ImGui::Image((void*)(intptr_t)fm_texture,
							ImVec2(fm_texture_w*factor, fm_texture_h*factor),
							ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));				
				ImGui::End();
			}
			
		
		
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
        clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
                        	
	}
	
	free_network(net);
		
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
	
}

