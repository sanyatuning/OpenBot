import importlib
import os
from subprocess import Popen, check_call
import sys

from aiohttp import web

from openbot import base_dir


async def init_frontend(app: web.Application):
    public_dir = os.getenv("OPENBOT_FRONTEND_PATH")

    async def handle_static(request: web.Request):
        path = request.match_info.get("path") or "index.html"
        if public_dir:
            return web.FileResponse(os.path.join(public_dir, path))
        return web.HTTPNotFound()

    app.router.add_get("/{path:.*}", handle_static)

    if os.getenv("FE_DEV"):
        run_frontend_dev_server()
        return

    if public_dir is None:
        public_dir = download_frontend()

    print("Frontend path:", public_dir)


def download_frontend():
    frontend_pkg = "openbot_frontend"
    version = get_pkg_version(frontend_pkg)

    installed = f"{frontend_pkg}=={version}"
    required = None

    for line in open(os.path.join(base_dir, "requirements_web.txt"), "r"):
        if line.startswith(frontend_pkg):
            required = line.strip()

    if required != installed:
        print("Installing frontend:", required)
        check_call([sys.executable, "-m", "pip", "install", required])

    import openbot_frontend

    importlib.reload(openbot_frontend)

    version = get_pkg_version(frontend_pkg)
    print("Running frontend:", version)

    return openbot_frontend.where()


def get_pkg_version(frontend_pkg):
    try:
        import importlib.metadata

        return importlib.metadata.version(frontend_pkg)
    except ModuleNotFoundError:
        pass

    import pkg_resources

    try:
        return pkg_resources.get_distribution(frontend_pkg).version
    except pkg_resources.DistributionNotFound:
        pass

    return None


def run_frontend_dev_server():
    if is_port_in_use(3000):
        return
    print("Start Frontend Development Server...")
    Popen(
        ["yarn", "run", "start"],
        cwd=os.path.join(base_dir, "frontend"),
    )


def is_port_in_use(port):
    import socket

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        return s.connect_ex(("localhost", port)) == 0
