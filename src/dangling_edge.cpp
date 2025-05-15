#include "dirent.h"
#include "fstream"
#include "queue"
#include "sys/stat.h"
#include "sys/types.h"
#include "thread"
#include "unistd.h"
#include "unordered_map"

#include "CGAL/Exact_predicates_inexact_constructions_kernel.h"
#include "CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h"
#include "CGAL/Surface_mesh.h"

#include "CGAL/Real_timer.h"
#include "CGAL/tags.h"

#include "args/args.hxx"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3> Mesh;
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
void computeDanglingEdge(std::vector<std::string> &stlFiles, size_t start,
                         size_t end) {

  for (size_t iter = start; iter < end; ++iter) {
    std::string inputFilename = stlFiles[iter];

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
        std::unordered_map<size_t, size_t> edge_weight;
        for (Mesh::Halfedge_index h : cmesh.halfedges()) {
            if (cmesh.is_border(h)) {
                // Skip the exterior halfedge, only count the weight of the edge for the interior halfedge
                continue;
            }
            size_t firstVert = cmesh.source(h);
            size_t secondVert = cmesh.target(h);
            long long lowerVert = static_cast<long long>(std::min(firstVert, secondVert));
            long long higherVert = static_cast<long long>(std::max(firstVert, secondVert));
            edge_weight[lowerVert * (1ll << 31) + higherVert] += 1;
        }

        // Get the scale in case of the scale is not aligned
        std::vector<double> max_point(3, -1e9);
        std::vector<double> min_point(3, 1e9);
        for (Mesh::Vertex_index v : cmesh.vertices()) {
            K::Point_3 current_point = cmesh.point(v);
            for (int dim = 0; dim < 3; ++dim) {
                max_point[dim] = std::max(max_point[dim], current_point[dim]);
                min_point[dim] = std::min(min_point[dim], current_point[dim]);
            }
        }
        double scale = -1.f;
        for (int dim = 0; dim < 3; ++dim) {
            scale = std::max(scale, max_point[dim] - min_point[dim]);
        }
        scale /= 2.0f;
        if (scale < 0.f) {
            std::cerr << "Error: normalized scale less than 0." << std::endl;
            return;
        }

        double danglingEdgeLength = 0.0f;
        std::unordered_map<size_t, size_t> index_map;
        std::vector<std::array<size_t, 2>> edges;
        size_t count = 0;
        for (const auto &pair : edge_weight) {
            if (pair.second == 1) {
                size_t firstVertIndex =
                    static_cast<size_t>(pair.first & ((1ll << 31) - 1ll));
                size_t secondVertIndex = static_cast<size_t>((pair.first >> 31));

                K::Point_3 vert1 = cmesh.point(Mesh::Vertex_index(firstVertIndex));
                K::Point_3 vert2 = cmesh.point(Mesh::Vertex_index(secondVertIndex));
                double edgeLength = std::sqrt((vert1 - vert2).squared_length());
                danglingEdgeLength += edgeLength;

                if (!index_map.count(firstVertIndex)) {
                    index_map[firstVertIndex] = count;
                    count += 1;
                }
                if (!index_map.count(secondVertIndex)) {
                    index_map[secondVertIndex] = count;
                    count += 1;
                }
                std::array<size_t, 2> edge {index_map[firstVertIndex], index_map[secondVertIndex]};
                edges.emplace_back(edge);
            }
        }
        danglingEdgeLength /= scale;

        // Create output directory path and filename
        std::string inputPath = inputFilename;
        std::string outputDir = get_parent_path(inputPath) + "_dangling_edge";
        std::string outputFilename =
            outputDir + "/" + replace_extension(get_filename(inputPath), ".txt");

        // Create output directory if it doesn't exist
        create_directories(outputDir);

        std::ofstream outFile(outputFilename);
        if (outFile.is_open()) {
            outFile << danglingEdgeLength;
            outFile.close();
            std::cout << "Dangling Edge Length saved to: " << outputFilename
                << std::endl;
        } else {
            std::cerr << "Error: Could not open file " << outputFilename
                << " for writing." << std::endl;
        }

    }  catch (const std::runtime_error &err) {
        std::cerr << "Error: " << err.what() << std::endl;
        std::cout << "Failed computing." << std::endl;
        return;
    }
  }
}

int main(int argc, char **argv) {

  // Configure the argument parser
  args::ArgumentParser parser("Dangling Edge Length");
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
        std::thread(computeDanglingEdge, std::ref(stlFiles), start, end));
  }

  for (size_t i = 0; i < numThreads; ++i) {
    if (workers[i].joinable()) {
      workers[i].join();
    }
  }

  return EXIT_SUCCESS;
}
