ARG BUILD_FROM=continuumio/miniconda3
FROM $BUILD_FROM

WORKDIR /opt/OpenBot/policy
COPY environment_linux.yml /opt/OpenBot/policy/
RUN conda env create -f environment_linux.yml

COPY frontend /opt/OpenBot/policy/frontend
COPY openbot /opt/OpenBot/policy/openbot
COPY requirements_web.txt /opt/OpenBot/policy/

# TODO replace with multi-stage build
RUN pip install openbot_frontend==0.7.0

CMD [ "bash", "-c", ". /opt/conda/etc/profile.d/conda.sh && conda activate openbot && exec python -u -m openbot.server" ]
