#include "sys/stat.h"
#include "sys/types.h"
#include "unistd.h"
#include "dirent.h"
#include "fstream"
#include "queue"
#include "thread"
#include "unordered_map"

#include "geometrycentral/surface/manifold_surface_mesh.h"
#include "geometrycentral/surface/meshio.h"
#include "geometrycentral/surface/vertex_position_geometry.h"

#include "geometrycentral/surface/direction_fields.h"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

#include "args/args.hxx"
#include "imgui.h"
#include <memory>

using namespace geometrycentral;
using namespace geometrycentral::surface;

std::string get_parent_path(const std::string& filepath) {
    size_t found = filepath.find_last_of("/\\");
    return (found != std::string::npos) ? filepath.substr(0, found) : "";
}

std::string get_filename(const std::string& filepath) {
    size_t found = filepath.find_last_of("/\\");
    return (found != std::string::npos) ? filepath.substr(found + 1) : filepath;
}

std::string replace_extension(const std::string& filename, const std::string& new_extension) {
    size_t found = filename.find_last_of(".");
    return (found != std::string::npos) ? filename.substr(0, found) + new_extension : filename + new_extension;
}

void create_directories(const std::string& dirPath) {
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

void computeMeshSegment(std::vector<std::string> &stlFiles, size_t start,
                        size_t end) {

  for (size_t iter = start; iter < end; ++iter) {
    std::string inputFilename = stlFiles[iter];

    // == Geometry-central data
    std::unique_ptr<ManifoldSurfaceMesh> manifold_mesh;
    std::unique_ptr<SurfaceMesh> surface_mesh;
    std::unique_ptr<VertexPositionGeometry> geometry;
    bool use_manifold = true;

    // Load mesh
    try {
      std::tie(manifold_mesh, geometry) = readManifoldSurfaceMesh(inputFilename);
    } catch (const std::runtime_error &e) {
      std::cerr << inputFilename << "throws Error: " << e.what() << std::endl;
      std::cout << "Manifold sanity check failed for .stl, try loading .ply"
                << std::endl;
      use_manifold = false;
      replaceSubstring(inputFilename, ".stl", ".ply");
      try {
          std::tie(surface_mesh, geometry) = readSurfaceMesh(inputFilename);
      } catch (const std::runtime_error &err) {
          std::cerr << inputFilename << "throws Error: " << e.what() << std::endl;
          std::cout << "Failed loading mesh." << std::endl;
          return;
      }
    }

    try {
        std::unique_ptr<geometrycentral::surface::SurfaceMesh> mesh;
        if (use_manifold) {
            mesh = std::move(manifold_mesh);
        } else {
            mesh = std::move(surface_mesh);
        }
        size_t nVert = mesh->nVertices();
        std::unordered_map<size_t, std::vector<size_t>> vertexGraph;
        const std::vector<std::vector<size_t>> &faceIndices =
            mesh->getFaceVertexList();

        for (std::vector<size_t> face : faceIndices) {

            size_t faceDegree = face.size();

            for (size_t i = 0; i < faceDegree; i++) {

                size_t vertex_A = face[i];
                size_t vertex_B = face[(i + 1) % faceDegree];

                vertexGraph[vertex_A].push_back(vertex_B);
            }
        }

        int set_number = 0;
        std::unordered_map<size_t, bool> isPerm;
        for (size_t iV = 0; iV < nVert; ++iV) {
            if (!isPerm.count(iV)) {
                set_number += 1;
                std::queue<size_t> vBFS;
                vBFS.push(iV);
                while (!vBFS.empty()) {
                    size_t currentVert = vBFS.front();
                    vBFS.pop();
                    for (const size_t &endV : vertexGraph[currentVert]) {
                        if (!isPerm.count(endV)) {
                            vBFS.push(endV);
                            isPerm[endV] = true;
                        }
                    }
                }
            }
        }

        // Create output directory path and filename
        std::string inputPath = inputFilename;
        std::string outputDir = get_parent_path(inputPath) + "_segment_num";
        std::string outputFilename = outputDir + "/" + replace_extension(get_filename(inputPath), ".txt");

        // Create output directory if it doesn't exist
        create_directories(outputDir);

        // Write set_number to the output file
        std::ofstream outFile(outputFilename);
        if (outFile.is_open()) {
            outFile << set_number;
            outFile.close();
            std::cout << "Segment number saved to: " << outputFilename << std::endl;
        } else {
            std::cerr << "Error: Could not open file " << outputFilename
                << " for writing." << std::endl;
        }
    } catch (const std::runtime_error &err) {
        std::cerr << "Error: " << err.what() << std::endl;
        std::cout << "Failed loading mesh." << std::endl;
        return;
    }
  }
}

// Written by Jingwei Xu for https://arxiv.org/abs/2411.04954
int main(int argc, char **argv) {

  // Configure the argument parser
  args::ArgumentParser parser("geometry-central & Polyscope example project");
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
        std::thread(computeMeshSegment, std::ref(stlFiles), start, end));
  }

  for (size_t i = 0; i < numThreads; ++i) {
    if (workers[i].joinable()) {
      workers[i].join();
    }
  }

  return EXIT_SUCCESS;
}
