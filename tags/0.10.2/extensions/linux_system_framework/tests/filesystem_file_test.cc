/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(FileSystem, GetInformation) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  ASSERT_TRUE(fi != NULL);

  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file.cc", fi->GetPath());
  EXPECT_EQ("file.cc", fi->GetName());
  EXPECT_EQ(4, fi->GetSize());
  EXPECT_GT(fi->GetDateLastModified().value, 0U);
  EXPECT_GT(fi->GetDateLastAccessed().value, 0U);

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, SetName) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  ASSERT_TRUE(fi != NULL);

  EXPECT_TRUE(fi->SetName("file2.cc"));
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file2.cc", fi->GetPath());
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));

  // SetName() doesn't support moving file.
  EXPECT_FALSE(fi->SetName("/tmp/file3.cc"));
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file2.cc", fi->GetPath());
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, GetParentFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  ASSERT_TRUE(fi != NULL);

  FolderInterface *folder = fi->GetParentFolder();
  EXPECT_EQ("/tmp/GGL_FileSystem_Test", folder->GetPath());
  folder->Destroy();

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, Delete) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  ASSERT_TRUE(fi != NULL);

  fi->Delete(true);
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, Copy) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  mkdir("/tmp/GGL_FileSystem_Test2", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  ASSERT_TRUE(fi != NULL);

  // Copies a file to another file.
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test/file2.cc", false));
  EXPECT_FALSE(fi->Copy("/tmp/GGL_FileSystem_Test/file2.cc", false));
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test/file2.cc", true));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));

  // Copies a file to another folder.
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test2", false));
  EXPECT_FALSE(fi->Copy("/tmp/GGL_FileSystem_Test2/", false));
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test2/", true));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file.cc"));

  // Copies a file to itself.
  EXPECT_FALSE(fi->Copy("/tmp/GGL_FileSystem_Test/file.cc", false));
  EXPECT_FALSE(fi->Copy("/tmp/GGL_FileSystem_Test/file.cc", true));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);
}

TEST(FileSystem, Move) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  mkdir("/tmp/GGL_FileSystem_Test2", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  ASSERT_TRUE(fi != NULL);

  // Moves a file to another file.
  EXPECT_TRUE(fi->Move("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file2.cc", fi->GetPath());

  // Moves a file to another folder.
  EXPECT_TRUE(fi->Move("/tmp/GGL_FileSystem_Test2/"));
  EXPECT_FALSE(fi->Move("/tmp/GGL_FileSystem_Test2/"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file2.cc"));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
