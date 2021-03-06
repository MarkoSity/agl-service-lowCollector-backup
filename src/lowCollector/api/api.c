/*
 * Copyright (C) 2016-2019 "IoT.bzh"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                            INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

#include <dlfcn.h>
#include "api.h"

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                        INITIALIZATION CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

json_object *api_plugin_init(userdata_t *userdata, json_object *arg)
{
  /* Variables definition */
  json_object *res;
  json_object *res_plugin;
  json_object *args;
  json_object *current_plugin;
  json_type args_type;
  int plugin_number;
  char *plugin_label;
  int loading_count;
  max_size_t Max_size;

  /* Allocate the answer of the function */
  res = json_object_new_object();
  res_plugin = json_object_new_object();

  /* Retrieve the max_size function from the collectd_glue library */
  Max_size = (max_size_t)dlsym(userdata->handle_collectd, MAX_SIZE_CHAR);
  if(!Max_size)
    return json_object_new_string(dlerror());

  /* Retrieve the j-son arguments */
  args = json_object_new_object();
  if(!json_object_object_get_ex(arg, PLUGIN_CHAR, &args))
    return json_object_new_string(ERR_ARG_CHAR);

  /* Retrieve the type of the j-son arguments */
  args_type = json_object_get_type(args);

  /* Here we allow multiple plugin initialization */
  current_plugin = json_object_new_object();

  /* Retrieve the number of plugin specified in the j-son arguments */
  switch(args_type)
  {
    case json_type_string:
    {
      plugin_number = 1;
			break;
    }

    case json_type_array:
    {
      plugin_number = json_object_array_length(args);
			break;
    }

    default:
      return json_object_new_string(ERR_ARG_CHAR);
  }

  /* Define the loading counter which is usefull to know whether at least one plugin has been initialized */
  loading_count = 0;

  /* For each one of the plugin notified in the j-son arguments */
  for(int i = 0 ; i != plugin_number ; i++)
  {
    /* Retrieve the current plugin */
    if(args_type == json_type_array)
      current_plugin = json_object_array_get_idx(args, i);

    else
      current_plugin = args;

    /* If the type ain't the one we expected we noticed it but we continue the process */
    if(!json_object_is_type(current_plugin, json_type_string))
			continue;

    /* Retrieve the string in the current j-son object */
    plugin_label = (char*)json_object_get_string(current_plugin);

    /* CPU plugin case */
    if(!strncmp(plugin_label, CPU_CHAR, (*Max_size)((size_t) 3, strlen(plugin_label))))
    {
      res_plugin = api_cpu_init(userdata);
      json_object_object_add(res, plugin_label, res_plugin);
      loading_count ++;
      continue;
    }

    /* Memory plugin case */
    else if(!strncmp(plugin_label, MEMORY_CHAR, (*Max_size)((size_t) 6, strlen(plugin_label))))
    {
      res_plugin = api_memory_init(userdata);
      json_object_object_add(res, plugin_label, res_plugin);
      loading_count ++;
      continue;
    }

    /* Processes plugin case */
    else if(!strncmp(plugin_label, PROCESSES_CHAR, (*Max_size)((size_t) 9, strlen(plugin_label))))
    {
      res_plugin = api_processes_init(userdata);
      json_object_object_add(res, plugin_label, res_plugin);
      loading_count ++;
      continue;
    }
  }

  /* If none of the previous plugins were known or already stored */
  if(!loading_count)
    return json_object_new_string(ERR_PLUGIN_UNKOWN);

  return res;
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                            CONFIGURATION CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

json_object *api_plugin_config(userdata_t *userdata, json_object *arg)
{
  /* Variables definition */
  json_object *args;

  /* Variable allocation */
  args = json_object_new_object();

  /* CPU plugin case */
  if(json_object_object_get_ex(arg, CPU_CHAR, &args))
    return api_cpu_config(userdata, args);

  /* Memory plugin case */
  else if(json_object_object_get_ex(arg, MEMORY_CHAR, &args))
    return api_memory_config(userdata, args);

  /* Processes plugin case */
  else if(json_object_object_get_ex(arg, PROCESSES_CHAR, &args))
    return api_processes_config(userdata, args);

  return json_object_new_string(ERR_PLUGIN_UNKOWN);

}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                            READ CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

json_object *api_plugin_read(userdata_t *userdata, json_object *arg)
{
  json_object *args;
  json_type args_type;
  char *plugin_label;
  max_size_t Max_size;

  Max_size = (max_size_t)dlsym(userdata->handle_collectd, MAX_SIZE_CHAR);
  if(!Max_size)
    return json_object_new_string(dlerror());

  args = json_object_new_object();
  plugin_label = (char*)malloc(sizeof(char));

  /* Invalid key */
  if(!json_object_object_get_ex(arg, PLUGIN_CHAR, &args))
    return json_object_new_string(ERR_ARG_CHAR);

  /* Retrieve the argument type */
  args_type = json_object_get_type(args);
  if(args_type != json_type_string)
    return json_object_new_string(ERR_ARG_CHAR);

  /* Retrieve the plugin_label from the j-son argument */
  plugin_label = (char*)json_object_get_string(args);

  /* CPU plugin case */
  if(!strncmp(plugin_label, CPU_CHAR, (*Max_size)((size_t) 3, strlen(plugin_label))))
    return api_cpu_read(userdata);

  /* Memory plugin case */
  else if(!strncmp(plugin_label, MEMORY_CHAR, (*Max_size)((size_t) 6, strlen(plugin_label))))
    return api_memory_read(userdata);

  /* Processes plugin case */
  else if(!strncmp(plugin_label, PROCESSES_CHAR, (*Max_size)((size_t) 9, strlen(plugin_label))))
    return api_processes_read(userdata);

  return json_object_new_string(ERR_PLUGIN_UNKOWN);
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                            RESET CALLBACK
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

json_object *api_plugin_reset(userdata_t *userdata, json_object *arg)
{
  /* Variables definition */
  json_object *res;
  json_object *res_plugin;
  json_object *args;
  json_object *current_plugin;
  json_type args_type;
  int plugin_number;
  char *plugin_label;
  int reset_count;
  max_size_t Max_size;
  plugin_list_t **plugin_list;

  /* Allocate the answer of the function */
  res = json_object_new_object();
  res_plugin = json_object_new_object();

  /* Retrieve the max_size function from the collectd_glue library */
  Max_size = (max_size_t)dlsym(userdata->handle_collectd, MAX_SIZE_CHAR);
  if(!Max_size)
    return json_object_new_string(dlerror());

  /* Retrieve the global plugin list variable from the collectd_glue library */
  plugin_list = (plugin_list_t **)dlsym(userdata->handle_collectd, PLUGIN_LIST_CHAR);
  if(!plugin_list)
    return json_object_new_string(dlerror());

  /* Ensure the plugin list is not NULL */
  if(!(*plugin_list))
    return json_object_new_string(ERR_PLUGIN_NULL_CHAR);

  /* Ensure the plugin list is not empty */
  if(!(*plugin_list)->size)
    return json_object_new_string(ERR_PLUGIN_EMPTY_CHAR);

  /* Retrieve the j-son arguments */
  args = json_object_new_object();
  if(!json_object_object_get_ex(arg, PLUGIN_CHAR, &args))
    return json_object_new_string(ERR_ARG_CHAR);

  /* Retrieve the type of the j-son arguments */
  args_type = json_object_get_type(args);

  /* Here we allow multiple plugin initialization */
  current_plugin = json_object_new_object();

  /* Retrieve the number of plugin specified in the j-son arguments */
  switch(args_type)
  {
    case json_type_string:
    {
      plugin_number = 1;
			break;
    }

    case json_type_array:
    {
      plugin_number = json_object_array_length(args);
			break;
    }

    default:
      return json_object_new_string(ERR_ARG_CHAR);
  }

  /* Define the reset counter which is usefull to know whether at least one plugin has been deleted */
  reset_count = 0;

  /* For each one of the plugin notified in the j-son arguments */
  for(int i = 0 ; i != plugin_number ; i++)
  {
    /* Retrieve the current plugin */
    if(args_type == json_type_array)
      current_plugin = json_object_array_get_idx(args, i);

    else
      current_plugin = args;

    /* If the type ain't the one we expected we noticed it but we continue the process */
    if(!json_object_is_type(current_plugin, json_type_string))
			continue;

    /* Retrieve the string in the current j-son object */
    plugin_label = (char*)json_object_get_string(current_plugin);

    /* CPU plugin case */
    if(!strncmp(plugin_label, CPU_CHAR, (*Max_size)((size_t) 3, strlen(plugin_label))))
    {
      res_plugin = api_cpu_reset(userdata);
      json_object_object_add(res, plugin_label, res_plugin);
      reset_count ++;
      continue;
    }

    /* Memory plugin case */
    if(!strncmp(plugin_label, MEMORY_CHAR, (*Max_size)((size_t) 6, strlen(plugin_label))))
    {
      res_plugin = api_memory_reset(userdata);
      json_object_object_add(res, plugin_label, res_plugin);
      reset_count ++;
      continue;
    }

    /* Processes plugin case */
    if(!strncmp(plugin_label, PROCESSES_CHAR, (*Max_size)((size_t) 6, strlen(plugin_label))))
    {
      res_plugin = api_processes_reset(userdata);
      json_object_object_add(res, plugin_label, res_plugin);
      reset_count ++;
      continue;
    }
  }

  /* If none of the previous plugins were known or already stored */
  if(!reset_count)
    return json_object_new_string(ERR_PLUGIN_NULL_CHAR);

  return res;
}
