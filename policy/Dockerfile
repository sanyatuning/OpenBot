FROM node:22 AS frontend

WORKDIR /frontend

COPY frontend/package.json frontend/yarn.lock ./
RUN yarn install

COPY frontend .
RUN yarn build

FROM continuumio/miniconda3:24.11.1-0

WORKDIR /opt/OpenBot/policy
COPY environment_linux.yml /opt/OpenBot/policy/
RUN conda env create -q -f environment_linux.yml

COPY openbot /opt/OpenBot/policy/openbot
COPY requirements_web.txt /opt/OpenBot/policy/

COPY --from=frontend /frontend/build /opt/OpenBot/policy/frontend
ENV OPENBOT_FRONTEND_PATH=/opt/OpenBot/policy/frontend

CMD [ "bash", "-c", ". /opt/conda/etc/profile.d/conda.sh && conda activate openbot && exec python -u -m openbot.server" ]
