#include "dirent.h"
#include "fstream"
#include "queue"
#include "set"
#include "sys/stat.h"
#include "sys/types.h"
#include "thread"
#include "unistd.h"

#include "CGAL/Exact_predicates_inexact_constructions_kernel.h"
#include "CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h"
#include "CGAL/Polygon_mesh_processing/self_intersections.h"
#include "CGAL/Surface_mesh.h"

#include "CGAL/Real_timer.h"
#include "CGAL/tags.h"

#include "args/args.hxx"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3> Mesh;
typedef boost::graph_traits<Mesh>::face_descriptor face_descriptor;
namespace PMP = CGAL::Polygon_mesh_processing;

std::string get_parent_path(const std::string &filepath) {
  size_t found = filepath.find_last_of("/\\");
  return (found != std::string::npos) ? filepath.substr(0, found) : "";
}

std::string get_filename(const std::string &filepath) {
  size_t found = filepath.find_last_of("/\\");
  return (found != std::string::npos) ? filepath.substr(found + 1) : filepath;
}

std::string replace_extension(const std::string &filename,
                              const std::string &new_extension) {
  size_t found = filename.find_last_of(".");
  return (found != std::string::npos)
             ? filename.substr(0, found) + new_extension
             : filename + new_extension;
}

void create_directories(const std::string &dirPath) {
  if (mkdir(dirPath.c_str(), 0755) && errno != EEXIST) {
    std::cerr << "Error creating directory: " << dirPath << std::endl;
  }
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

bool isStlFile(const std::string &filename) {
  return filename.rfind(".stl") == (filename.size() - 4);
}

void replaceSubstring(std::string &str, const std::string &from,
                      const std::string &to) {
  size_t startPos = str.find(from);
  if (startPos != std::string::npos) {
    str.replace(startPos, from.length(), to);
  }
}

std::vector<std::string> list_directory(const std::string &dirPath) {
  std::vector<std::string> filenames;
  DIR *dir = opendir(dirPath.c_str());

  if (dir == nullptr) {
    std::cerr << "Error opening directory: " << dirPath << std::endl;
    return filenames;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string filename(entry->d_name);
    if (filename != "." && filename != "..") {
      filenames.push_back(filename);
    }
  }

  closedir(dir);
  return filenames;
}

// Written by Jingwei Xu for https://arxiv.org/abs/2411.04954
void computeSelfIntersection(std::vector<std::string> &stlFiles, size_t start,
                             size_t end) {

  for (size_t iter = start; iter < end; ++iter) {
    std::string inputFilename = stlFiles[iter];
    // Create output directory path and filename
    std::string inputPath = inputFilename;
    std::string outputDir = get_parent_path(inputPath) + "_self_intersection";
    std::string outputFilename =
            outputDir + "/" + replace_extension(get_filename(inputPath), ".txt");
    if (fileExists(outputFilename)) {
        std::cout << outputFilename + " exists!" << std::endl;
        continue;
    }

    Mesh cmesh;
    if (!PMP::IO::read_polygon_mesh(inputFilename, cmesh) ||
        !CGAL::is_triangle_mesh(cmesh)) {
      std::cerr << "Can't open stl file. Try ply file instead." << std::endl;
      replaceSubstring(inputFilename, ".stl", ".ply");
      if (!PMP::IO::read_polygon_mesh(inputFilename, cmesh) ||
          !CGAL::is_triangle_mesh(cmesh)) {
        std::cerr << "Invalid data." << std::endl;
      } else {
        return;
      }
    }

    try {
      std::cout << "Face number:" << cmesh.faces().size() << std::endl;
      std::cout << "Using parallel mode? "
                << std::is_same<CGAL::Parallel_if_available_tag,
                                CGAL::Parallel_tag>::value
                << std::endl;
      CGAL::Real_timer timer;
      timer.start();
      bool intersecting =
          PMP::does_self_intersect<CGAL::Parallel_if_available_tag>(
              cmesh, CGAL::parameters::vertex_point_map(
                         get(CGAL::vertex_point, cmesh)));
      std::cout << (intersecting ? "There are self-intersections."
                                 : "There is no self-intersection.")
                << std::endl;
      std::cout << "Elapsed time (does self intersect): " << timer.time()
                << std::endl;
      timer.reset();
      std::vector<std::pair<face_descriptor, face_descriptor>> intersected_tris;
      PMP::self_intersections<CGAL::Parallel_if_available_tag>(
          faces(cmesh), cmesh, std::back_inserter(intersected_tris));
      std::set<CGAL::SM_Face_index> sface;
      for (auto p : intersected_tris) {
        sface.insert(p.first);
        sface.insert(p.second);
      }
      size_t self_intersect_faces_num = sface.size();
      size_t faces_num = cmesh.num_faces();
      std::cout << intersected_tris.size() << " pairs of triangles intersect."
                << std::endl;
      std::cout << "Elapsed time (self intersections): " << timer.time()
                << std::endl;

      // Create output directory if it doesn't exist
      create_directories(outputDir);

      std::ofstream outFile(outputFilename);
      if (outFile.is_open()) {
        outFile << self_intersect_faces_num << '\n' << faces_num;
        outFile.close();
        std::cout << "Self intersection saved to: " << outputFilename
                  << std::endl;
      } else {
        std::cerr << "Error: Could not open file " << outputFilename
                  << " for writing." << std::endl;
      }
    } catch (const std::runtime_error &err) {
      std::cerr << "Error: " << err.what() << std::endl;
      std::cout << "Failed computing." << std::endl;
      return;
    }
  }
}

int main(int argc, char **argv) {

  // Configure the argument parser
  args::ArgumentParser parser("Self Intersection");
  args::Positional<std::string> inputDirname(parser, "mesh_dir",
                                             "Directory contains mesh files.");

  // Parse args
  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help &h) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  // Make sure a mesh name was given
  if (!inputDirname) {
    std::cerr << "Please specify a mesh file as argument" << std::endl;
    return EXIT_FAILURE;
  }

  std::string dirPath = args::get(inputDirname);

  std::vector<std::string> files = list_directory(dirPath);

  dirPath.push_back('/');
  std::vector<std::string> stlFiles;
  for (std::string s : files) {
    if (isStlFile(s)) {
      stlFiles.push_back(dirPath + s);
    }
  }

  size_t numThreads = std::thread::hardware_concurrency();
  size_t batch = stlFiles.size() / numThreads + 1;
  std::vector<std::thread> workers;
  for (size_t i = 0; i < numThreads; ++i) {
    size_t start = i * batch;
    size_t end = std::min(start + batch, stlFiles.size());
    workers.push_back(
        std::thread(computeSelfIntersection, std::ref(stlFiles), start, end));
  }

  for (size_t i = 0; i < numThreads; ++i) {
    if (workers[i].joinable()) {
      workers[i].join();
    }
  }

  return EXIT_SUCCESS;
}
